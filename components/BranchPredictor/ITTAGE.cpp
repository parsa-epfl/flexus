#include "ITTAGE.h"
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <set>

// initialize the predictor
ITTAGE::ITTAGE()
{
    // initialize branch target predictor
    double alpha = pow((double)MAX_HIST / (double)MIN_HIST, 1.0 / (ITTAGE_NTAB-1));
    for(int i=0; i<ITTAGE_NTAB; i++)
    {
        ittage_hlen[i] = (int)((MIN_HIST * pow(alpha, i)) + 0.5);
    }
}

// indirect predictor lookup
uint64_t ITTAGE::predict(uint64_t ip)
{
    ittage_hit_bank = -1;
    ittage_alt_diff =  0;

    // indirect predictor access
    const int idx_width = ITTAGE_IDX_NBIT;
    const int tag_width = ITTAGE_TAG_NBIT;

    // folding history
    for(int i=0; i<ITTAGE_NTAB; i++)
    {
        uint64_t ghr = 0;
        ittage_hash_idx[i] = (ip >> 2) ^ (ip >> (3+i));
        ittage_hash_tag[i] = (ip >> 2) ^ (ip << i);
        for(int h=0; h<ittage_hlen[i]; h+=idx_width)
        {
            ghr = (btb_ghr[h / 32] >> (h % 32));
            if(ittage_hlen[i] < (h+idx_width)) ghr &= ((1 << (ittage_hlen[i]-h)) - 1);
            ittage_hash_idx[i] ^= ghr;
        }
        for(int h=0; h<ittage_hlen[i]; h+=tag_width)
        {
            ghr = (btb_ghr[h / 32] >> (h % 32));
            if(ittage_hlen[i] < (h+tag_width)) ghr &= ((1 << (ittage_hlen[i]-h)) - 1);
            ittage_hash_tag[i] ^= ghr;
        }
        for(int h=0; h<ittage_hlen[i]; h+=tag_width-1)
        {
            ghr = (btb_ghr[h / 32] >> (h % 32));
            if(ittage_hlen[i] < (h+tag_width-1)) ghr &= ((1 << (ittage_hlen[i]-h)) - 1);
            ittage_hash_tag[i] ^= ghr << 1;
        }
        ittage_hash_idx[i] &= (1 << idx_width) - 1;
        ittage_hash_tag[i] &= (1 << tag_width) - 1;
    }

    uint64_t ittage_alt_pred = 0;
    uint64_t ittage_hit_pred = 0;
    uint64_t confidence = 0;
    for(int i=0; i<ITTAGE_NTAB; i=i+1)
    {
        if(ittage_set[i][ittage_hash_idx[i]].tag == ittage_hash_tag[i])
        {
            ittage_alt_pred = ittage_hit_pred;
            ittage_hit_pred = ittage_set[i][ittage_hash_idx[i]].target;
            confidence = ittage_set[i][ittage_hash_idx[i]].confidence;
            ittage_hit_bank = i;
        }
    }
    ittage_alt_diff = ittage_hit_pred != ittage_alt_pred;
    uint64_t target = ( (confidence == 0) && (ittage_use_alt >= 0) ) ? ittage_alt_pred : ittage_hit_pred;
    return target;
}

// indirect predictor update
uint64_t ITTAGE::update_target(uint64_t target, bool mispred, const BPredState& aBPState)
{
    // save current variables
    // branch history
    uint64_t tmp_btb_ghr[16];
    
    // ITTAGE
    int tmp_ittage_hlen[ITTAGE_NTAB];
    int64_t tmp_ittage_tick;
    int64_t tmp_ittage_hit_bank;
    int64_t tmp_ittage_alt_diff;
    int64_t tmp_ittage_use_alt;
    uint64_t tmp_ittage_hit_pred;
    uint64_t tmp_ittage_alt_pred;
    uint64_t tmp_ittage_hash_idx[ITTAGE_NTAB];
    uint64_t tmp_ittage_hash_tag[ITTAGE_NTAB];

    for (int i = 0; i < 16; i++){
        tmp_btb_ghr[i] = btb_ghr[i];
    }
    for (int i = 0; i < ITTAGE_NTAB; i++){
        tmp_ittage_hlen[i] = ittage_hlen[i];
        tmp_ittage_hash_idx[i] = ittage_hash_idx[i];
        tmp_ittage_hash_tag[i] = ittage_hash_tag[i];
    }
    
    tmp_ittage_tick = ittage_tick;
    tmp_ittage_hit_bank = ittage_hit_bank;
    tmp_ittage_alt_diff = ittage_alt_diff;
    tmp_ittage_use_alt = ittage_use_alt;
    tmp_ittage_hit_pred = ittage_hit_pred;
    tmp_ittage_alt_pred = ittage_alt_pred;

    // restore the state from aBPState
    restore_history(aBPState);

    // predict again to reconstruct state
    uint64_t pred_tgt = predict(aBPState.pc);

    // hitting entry update
    if(ittage_hit_bank >= 0)
    {
        ITTAGE_entry& ittage_entry = ittage_set[ittage_hit_bank][ittage_hash_idx[ittage_hit_bank]];

        // update use_alt
        if(ittage_entry.confidence == 0)
        {
            if(ittage_alt_diff)
            {
                if(ittage_alt_pred == target) ittage_use_alt++;
                if(ittage_hit_pred == target) ittage_use_alt--;
                if(ittage_use_alt >  7) ittage_use_alt =  7;
                if(ittage_use_alt < -8) ittage_use_alt = -8;
            }
        }

        // update confidence and RRPV
        if(ittage_entry.target == target)
        {
            if(ittage_alt_diff)
            {
                if(ittage_entry.rrpv < 3) ittage_entry.rrpv++;
            }
            if(ittage_entry.confidence < 3)
            {
                ittage_entry.confidence++;
            }
        }
        else
        {
            if(ittage_entry.confidence > 0)
            {
                ittage_entry.confidence--;
            }
            else
            {
                ittage_entry.target = target;
            }
        }
    }

    // allocation
    if(mispred && (ittage_hit_pred != target))
    {
        for(int64_t i=ittage_hit_bank+1; i<ITTAGE_NTAB; i++)
        {
            if(ittage_set[i][ittage_hash_idx[i]].rrpv > 3)
            {
                ittage_set[i][ittage_hash_idx[i]].rrpv = 0;
            }
            if(rand() & 3) // skip 25%
            {
                if(ittage_set[i][ittage_hash_idx[i]].rrpv == 0)
                {
                    // allocate
                    ittage_set[i][ittage_hash_idx[i]].tag = ittage_hash_tag[i];
                    ittage_set[i][ittage_hash_idx[i]].target = target;
                    ittage_set[i][ittage_hash_idx[i]].confidence = 0;
                    ittage_set[i][ittage_hash_idx[i]].rrpv = 0;
                    ittage_tick -= 1;
                    if(ittage_tick < 0) ittage_tick = 0;
                    break;
                }
                else
                {
                    // fail to allocate
                    ittage_tick += 1;
                }
            }
        }

        if(ittage_tick > 500)
        {
            // reset useful
            for(int i=0; i<ITTAGE_NTAB; i++)
            {
                for(int j=0; j<ITTAGE_IDX_SIZE; j++)
                {
                    if(ittage_set[i][j].rrpv > 0) ittage_set[i][j].rrpv--;
                }
            }
            ittage_tick = 0;
        }
    }

    // restore current history
    for (int i = 0; i < 16; i++){
        btb_ghr[i] = tmp_btb_ghr[i];
    }
    for (int i = 0; i < ITTAGE_NTAB; i++){
        ittage_hlen[i] = tmp_ittage_hlen[i];
        ittage_hash_idx[i] = tmp_ittage_hash_idx[i];
        ittage_hash_tag[i] = tmp_ittage_hash_tag[i];
    }
    
    ittage_tick = tmp_ittage_tick;
    ittage_hit_bank = tmp_ittage_hit_bank;
    ittage_alt_diff = tmp_ittage_alt_diff;
    ittage_use_alt = tmp_ittage_use_alt;
    ittage_hit_pred = tmp_ittage_hit_pred;
    ittage_alt_pred = tmp_ittage_alt_pred;

    return pred_tgt;
}

// update branch predictor history
void ITTAGE::update_history(uint64_t target)
{
    // branch target history hash computation
    uint64_t hash = (target >> 2) & 0xffULL ;
    for(int i=0; i<16; i=i+1) // 16 * 32 = 512-bit total
    {
        btb_ghr[i] <<= 2;
        btb_ghr[i] ^= hash;
        hash = (btb_ghr[i] >> 32) & 3;
    }
}

void ITTAGE::checkpointHistory(BPredState &aBPState) const{
    for (int i = 0; i < 16; i++){
        aBPState.btb_ghr[i] = btb_ghr[i];
    }
    for (int i = 0; i < ITTAGE_NTAB; i++){
        aBPState.ittage_hlen[i] = ittage_hlen[i];
        aBPState.ittage_hash_idx[i] = ittage_hash_idx[i];
        aBPState.ittage_hash_tag[i] = ittage_hash_tag[i];
    }
    
    aBPState.ittage_tick = ittage_tick;
    aBPState.ittage_hit_bank = ittage_hit_bank;
    aBPState.ittage_alt_diff = ittage_alt_diff;
    aBPState.ittage_use_alt = ittage_use_alt;
    aBPState.ittage_hit_pred = ittage_hit_pred;
    aBPState.ittage_alt_pred = ittage_alt_pred;
    aBPState.theITTAGEHistoryValid = true;
}

void ITTAGE::restore_history(const BPredState &aBPState){
    if (!aBPState.theITTAGEHistoryValid) {
        return;
    }

    for (int i = 0; i < 16; i++){
        btb_ghr[i] = aBPState.btb_ghr[i];
    }
    for (int i = 0; i < ITTAGE_NTAB; i++){
        ittage_hlen[i] = aBPState.ittage_hlen[i]; 
        ittage_hash_idx[i] = aBPState.ittage_hash_idx[i];
        ittage_hash_tag[i] = aBPState.ittage_hash_tag[i];
    }
    
    ittage_tick = aBPState.ittage_tick;
    ittage_hit_bank = aBPState.ittage_hit_bank;
    ittage_alt_diff = aBPState.ittage_alt_diff;
    ittage_use_alt = aBPState.ittage_use_alt;
    ittage_hit_pred = aBPState.ittage_hit_pred;
    ittage_alt_pred = aBPState.ittage_alt_pred;
}