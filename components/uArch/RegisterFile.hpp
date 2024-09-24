
#ifndef FLEXUS_uARCH_REGISTERFILE_HPP_INCLUDED
#define FLEXUS_uARCH_REGISTERFILE_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <core/performance/profile.hpp>
#include <list>
#include <vector>

namespace nuArch {

class RegisterFile
{
  protected:
    std::vector<std::vector<std::list<InstructionDependance>>> theDependances;
    std::vector<std::vector<eResourceStatus>> theStatus;
    std::vector<std::vector<register_value>> theRegs;
    std::vector<std::vector<int32_t>> theCollectCounts;

  public:
    void initialize(std::vector<uint32_t> const& aSizes)
    {
        theDependances.resize(aSizes.size());
        theStatus.resize(aSizes.size());
        theRegs.resize(aSizes.size());
        theCollectCounts.resize(aSizes.size());

        for (uint32_t i = 0; i < aSizes.size(); ++i) {
            theDependances[i].resize(aSizes[i]);
            theStatus[i].resize(aSizes[i]);
            theRegs[i].resize(aSizes[i]);
            theCollectCounts[i].resize(aSizes[i], 10);
        }

        reset();
    }

    void reset()
    {
        FLEXUS_PROFILE();
        for (uint32_t i = 0; i < theDependances.size(); ++i) {
            for (auto& aDependance : theDependances[i]) {
                aDependance.clear();
            }
            for (auto& aStatus : theStatus[i]) {
                aStatus = kUnmapped;
            }
            for (auto& aReg : theRegs[i]) {
                aReg = (uint64_t)0ULL;
            }
        }
    }

    void collectAll()
    {
        FLEXUS_PROFILE();
        for (uint32_t i = 0; i < theDependances.size(); ++i) {
            for (uint32_t j = 0; j < theDependances[i].size(); ++j) {
                theCollectCounts[i][j] = 10;

                std::list<InstructionDependance>::iterator iter, temp, end;
                iter = theDependances[i][j].begin();
                end  = theDependances[i][j].end();
                while (iter != end) {
                    temp = iter;
                    ++iter;
                    if (temp->instruction->isComplete()) { theDependances[i][j].erase(temp); }
                }
            }
        }
    }

    void collect(mapped_reg aReg)
    {
        FLEXUS_PROFILE();
        if (--theCollectCounts[aReg.theType][aReg.theIndex] <= 0) {
            theCollectCounts[aReg.theType][aReg.theIndex] = 10;

            std::list<InstructionDependance>::iterator iter, temp, end;
            iter = theDependances[aReg.theType][aReg.theIndex].begin();
            end  = theDependances[aReg.theType][aReg.theIndex].end();
            while (iter != end) {
                temp = iter;
                ++iter;
                if (temp->instruction->isComplete()) { theDependances[aReg.theType][aReg.theIndex].erase(temp); }
            }
        }
    }
    void map(mapped_reg aReg)
    {
        FLEXUS_PROFILE();
        theStatus[aReg.theType][aReg.theIndex] = kNotReady;
        DBG_Assert(theDependances[aReg.theType][aReg.theIndex].empty());
    }
    void squash(mapped_reg aReg, uArch& aCore)
    {
        FLEXUS_PROFILE();
        if (theStatus[aReg.theType][aReg.theIndex] != kUnmapped) { theStatus[aReg.theType][aReg.theIndex] = kNotReady; }
        aCore.squash(theDependances[aReg.theType][aReg.theIndex]);
    }
    void unmap(mapped_reg aReg)
    {
        FLEXUS_PROFILE();
        theStatus[aReg.theType][aReg.theIndex] = kUnmapped;
        theDependances[aReg.theType][aReg.theIndex].clear();
    }
    eResourceStatus status(mapped_reg aReg) { return theStatus[aReg.theType][aReg.theIndex]; }

    void setStatus(mapped_reg aReg, eResourceStatus aStatus) { theStatus[aReg.theType][aReg.theIndex] = aStatus; }

    eResourceStatus request(mapped_reg aReg, InstructionDependance const& aDependance)
    {
        FLEXUS_PROFILE();
        collect(aReg);
        theDependances[aReg.theType][aReg.theIndex].push_back(aDependance);
        return theStatus[aReg.theType][aReg.theIndex];
    }

    // Write a value into the register file without side-effects
    void poke(mapped_reg aReg, register_value aValue, bool isW = false)
    {
        if (isW) {
            register_value val                   = theRegs[aReg.theType][aReg.theIndex];
            uint64_t res                         = boost::get<uint64_t>(val) & 0xffffffff00000000;
            res                                  = (boost::get<uint64_t>(aValue) & 0xffffffff);
            register_value final                 = res;
            theRegs[aReg.theType][aReg.theIndex] = final;
        } else {
            theRegs[aReg.theType][aReg.theIndex] = aValue;
        }
    }

    register_value peek(mapped_reg aReg) { return theRegs[aReg.theType][aReg.theIndex]; }

    register_value read(mapped_reg aReg)
    {
        DBG_Assert(theStatus[aReg.theType][aReg.theIndex] == kReady);
        return peek(aReg);
    }

    void write(mapped_reg aReg, register_value aValue, uArch& aCore, bool isW)
    {
        FLEXUS_PROFILE();
        poke(aReg, aValue, isW);
        theStatus[aReg.theType][aReg.theIndex] = kReady;

        std::list<InstructionDependance>::iterator iter = theDependances[aReg.theType][aReg.theIndex].begin();
        std::list<InstructionDependance>::iterator end  = theDependances[aReg.theType][aReg.theIndex].end();
        std::list<InstructionDependance>::iterator tmp;
        while (iter != end) {
            tmp = iter;
            ++iter;
            if (tmp->instruction->isComplete()) {
                theDependances[aReg.theType][aReg.theIndex].erase(tmp);
            } else {
                tmp->satisfy();
            }
        }
    }
};

} // namespace nuArchARM

#endif // FLEXUS_uARCH_REGISTERFILE_HPP_INCLUDED
