#ifndef FLEXUS_TRACER_DISPATCHER_HPP
#define FLEXUS_TRACER_DISPATCHER_HPP


// Define a macro that identify instruction from the kernel space
// do not confuse with EL1.
#define IS_KERNEL(logical_address) ((logical_address & 0xf000000000000000ULL) != 0)

#include "TracerStat.hpp"
#include "core/types.hpp"

using namespace Flexus::Qemu;
using namespace Flexus::SharedTypes;
using namespace nDecoupledFeeder;

/**
 * Static class which 'just' hold the function to dispatch
 * the instruction type along their respective data path
 *
 *  Bryan Perdrizat
 *      As a rule of thumb, every statistic operation for the feeder
 *      should happen here.
 **/
class TracerDispatcher {

public:
    /**
     * Return true if the memory access is accessing a
     * memory mapped IO
     **/
    static bool
    dispatch_io(memory_transaction_t& transaction)
    {
        return (transaction.io);
    }

    /**
     * Return memory message for a fetch request
     * TODO: fix eBranchType enum, it's stupid
     **/
    static void
    dispatch_instruction(memory_transaction_t& transaction, MemoryMessage& msg)
    {
        eBranchType branchTypeTable[8] = {
            kNonBranch,   kConditional,  kUnconditional, kCall,
            kIndirectReg, kIndirectCall, kReturn,        kLastBranchType};

        msg.type() = MemoryMessage::FetchReq;
        msg.tl() = 0;
        msg.reqSize() = 4;
        msg.branchType() = branchTypeTable[transaction.s.branch_type];
        msg.branchAnnul() = (transaction.s.annul != 0);
    }

    /**
     * Dispatch atomic READ-MODIFY-WRITE instruction
     **/
    static bool
    dispatch_rmw(memory_transaction_t& transaction, MemoryMessage& msg)
    {
        if (transaction.s.atomic)
        {
            msg.type() = MemoryMessage::RMWReq;
            return true;
        }
        return false;
    }

    /**
     * Set message type based on the memory transaction dispatch
     **/
    static bool
    dispatch_memory_access(
        TracerStat& counter,
        memory_transaction_t& transaction,
        MemoryMessage& msg)
    {

        switch(transaction.s.type)
        {
            case API::QEMU_Trans_Load:
                counter.touch_feeder_load(msg.priv());
                msg.type() = MemoryMessage::LoadReq;
                return true;

            case API::QEMU_Trans_Prefetch:
                counter.touch_feeder_prefetch(msg.priv());
                msg.type() = MemoryMessage::LoadReq;
                return true;

            case API::QEMU_Trans_Store:
                counter.touch_feeder_store(msg.priv());
                msg.type() = MemoryMessage::StoreReq;
                return true;

            case API::QEMU_Trans_Instr_Fetch:
                dispatch_instruction(transaction, msg);
                counter.touch_feeder_fetch(msg.priv());
                return true;

            case API::QEMU_Trans_Cache:
            default:
                return false;
        }

        return true;
    }


    /**
     * Entry point of the dispatch
     * If any dispatch return false, or encounter an unexpected behaviour
     * this should be stopped here at worse.
     **/
    static void
    dispatch(
            TracerStat& counter,
            memory_transaction_t& transaction,
            MemoryMessage& msg)
    {
        msg.address() = PhysicalMemoryAddress(transaction.s.physical_address);
        msg.pc() = VirtualMemoryAddress(transaction.s.logical_address);
        msg.priv() = IS_KERNEL(transaction.s.logical_address);

        bool ret = false;
        ret = dispatch_io(transaction);
        if (ret)
        {
            counter.touch_feeder_io(msg.priv());
            return;
        }

        ret = dispatch_rmw(transaction, msg);
        if (ret)
        {
            counter.touch_feeder_rmw(msg.priv());
            return;
        }

        ret = dispatch_memory_access(counter, transaction, msg);
        if (!ret) DBG_Assert(false);
    }
};


#endif