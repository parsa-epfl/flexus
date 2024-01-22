//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#ifndef FLEXUS_uARCH_microARCH_HPP_INCLUDED
#define FLEXUS_uARCH_microARCH_HPP_INCLUDED

#include <functional>
#include <memory>

#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/CommonQEMU/Slices/PredictorMessage.hpp> /* CMU-ONLY */
#include <components/CommonQEMU/Translation.hpp>

namespace Flexus {
namespace SharedTypes {
struct BranchFeedback;
}
} // namespace Flexus

namespace nuArch {

using Flexus::SharedTypes::VirtualMemoryAddress;

struct microArch {
  static std::shared_ptr<microArch>
  construct(uArchOptions_t options, std::function<void(eSquashCause)> squash,
            std::function<void(VirtualMemoryAddress)> redirect,
            std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
            std::function<void(bool)> aStoreForwardingHitFunction,
            std::function<void(int32_t)> mmuResyncFunction);

  virtual int32_t availableROB() = 0;
  virtual const uint32_t core() const = 0;
  virtual bool isSynchronized() = 0;
  virtual bool isQuiesced() = 0;
  virtual bool isStalled() = 0;
  virtual bool isHalted() = 0;
  virtual int32_t iCount() = 0;
  virtual void dispatch(boost::intrusive_ptr<AbstractInstruction>) = 0;
  virtual void skipCycle() = 0;
  virtual void cycle() = 0;
  virtual void issueMMU(TranslationPtr aTranslation) = 0;
  virtual void pushMemOp(boost::intrusive_ptr<MemOp>) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual TranslationPtr popTranslation() = 0;
  virtual void pushTranslation(TranslationPtr aTranslation) = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize,
                                  uint64_t marker) = 0;
  virtual int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize) = 0;
  virtual bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction) = 0;
  virtual void clearExclusiveLocal() = 0;
  virtual ~microArch() {
  }
  virtual void testCkptRestore() = 0;
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;
  virtual void pregs() = 0;
  virtual void pregsAll() = 0;
  virtual void resynchronize(bool was_expected) = 0;
  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;
  virtual void writePermissionLost(PhysicalMemoryAddress anAddress) = 0;
};

} // namespace nuArchARM

#endif // FLEXUS_uARCH_microARCH_HPP_INCLUDED
