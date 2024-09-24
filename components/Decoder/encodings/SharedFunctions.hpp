
#ifndef FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED

#include "../BitManip.hpp"
#include "../Constraints.hpp"
#include "../Effects.hpp"
#include "../Instruction.hpp"
#include "../Interactions.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "../Validations.hpp"

#include <bitset>
#include <boost/none.hpp>
#include <boost/throw_exception.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <iomanip>
#include <iostream>
#include <list>

namespace nDecoder {
using namespace nuArch;

eExtendType
DecodeRegExtend(int anOption);
eIndex
getIndex(unsigned int index);
std::unique_ptr<Operation>
extend(eExtendType anExtend);

void
addReadCC(SemanticInstruction* inst, int32_t anOpNumber, std::list<InternalDependance>& dependances, bool is_64);
void
addSetCC(SemanticInstruction* inst, predicated_action& exec, bool is64);
void
addReadXRegister(SemanticInstruction* inst,
                 int32_t anOpNumber,
                 uint32_t rs,
                 std::list<InternalDependance>& dependances,
                 bool is_64);
void
readRegister(SemanticInstruction* inst,
             int32_t anOpNumber,
             uint32_t rs,
             std::list<InternalDependance>& dependances,
             bool is_64);
// void addReadRDs(SemanticInstruction * inst, uint32_t rd, uint32_t rd1 );
void
addReadConstant(SemanticInstruction* inst,
                int32_t anOpNumber,
                uint64_t val,
                std::list<InternalDependance>& dependances);
void
addAnnulment(SemanticInstruction* inst, predicated_action& exec, InternalDependance const& aWritebackDependance);
void
addRD1Annulment(SemanticInstruction* inst, predicated_action& exec, InternalDependance const& aWritebackDependance);
void
addWriteback(SemanticInstruction* inst,
             eOperandCode aRegisterCode,
             eOperandCode aMappedRegisterCode,
             predicated_action& exec,
             bool a64,
             bool setflags,
             bool addSquash = true);
void
addWriteback1(SemanticInstruction* inst,
              eOperandCode aRegisterCode,
              eOperandCode aMappedRegisterCode,
              predicated_action& exec,
              bool a64,
              bool setflags,
              bool addSquash = true);
void
addWriteback2(SemanticInstruction* inst,
              eOperandCode aRegisterCode,
              eOperandCode aMappedRegisterCode,
              predicated_action& exec,
              bool a64,
              bool setflags,
              bool addSquash = true);

// Destination
void
addDestination(SemanticInstruction* inst,
               uint32_t rd,
               predicated_action& exec,
               bool is64,
               bool setflags  = false,
               bool addSquash = true);
void
addDestination1(SemanticInstruction* inst,
                uint32_t rd,
                predicated_action& exec,
                bool is64,
                bool setflags  = false,
                bool addSquash = true);
void
addDestination2(SemanticInstruction* inst,
                uint32_t rd,
                predicated_action& exec,
                bool is64,
                bool setflags  = false,
                bool addSquash = true);
void
addPairDestination(SemanticInstruction* inst,
                   uint32_t rd,
                   uint32_t rd1,
                   predicated_action& exec,
                   bool is64,
                   bool addSquash = true);
simple_action
addAddressCompute(SemanticInstruction* inst, std::vector<std::list<InternalDependance>>& rs_deps);
void
setRD(SemanticInstruction* inst, uint32_t rd);
void
setRD1(SemanticInstruction* inst, uint32_t rd1);
void
setRD2(SemanticInstruction* inst, uint32_t rd2);
void
setRS(SemanticInstruction* inst, eOperandCode rs_code, uint32_t rs);

// aux
predicated_action
addExecute(SemanticInstruction* inst,
           std::unique_ptr<Operation> anOperation,
           std::vector<std::list<InternalDependance>>& rs_deps,
           eOperandCode aResult                  = kResult,
           boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));
predicated_action
addExecute(SemanticInstruction* inst,
           std::unique_ptr<Operation> anOperation,
           std::vector<eOperandCode> anOperands,
           std::vector<std::list<InternalDependance>>& rs_deps,
           eOperandCode aResult                  = kResult,
           boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));

void
MEMBAR(SemanticInstruction* inst, uint32_t anAccess);
void
satisfyAtDispatch(SemanticInstruction* inst, std::list<InternalDependance>& dependances);

uint64_t
bitmask64(unsigned int length);
uint64_t
bitfield_replicate(uint64_t mask, unsigned int e);

uint32_t
highestSetBit(bits val);
uint64_t
ones(uint64_t length);
uint64_t
ror(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t
lsl(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t
lsr(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t
asr(uint64_t input, uint64_t input_size, uint64_t shift_size);

archinst
generateException(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, eExceptionType aType);

bool
decodeBitMasks(uint64_t& tmask, uint64_t& wmask, bool immN, char imms, char immr, bool immediate, uint32_t dataSize);
bool
disas_ldst_compute_iss_sf(int size, bool is_signed, int opc);
} // namespace nDecoder

#endif // FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED