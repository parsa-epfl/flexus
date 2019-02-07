// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#ifndef FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED

#include <list>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/none.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

#include "../armInstruction.hpp"
#include "../SemanticInstruction.hpp"
#include "../SemanticActions.hpp"
#include "../Effects.hpp"
#include "../Validations.hpp"
#include "../Constraints.hpp"
#include "../Interactions.hpp"
#include "../armBitManip.hpp"
#include <components/uArchARM/uArchInterfaces.hpp>


namespace narmDecoder {
using namespace nuArchARM;

eExtendType DecodeRegExtend(int anOption);
eIndex getIndex ( unsigned int index);
std::unique_ptr<Operation> extend(eExtendType anExtend);

void addReadXRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances, bool is_64);
//void addReadRDs(SemanticInstruction * inst, uint32_t rd, uint32_t rd1 );
void addReadConstant (SemanticInstruction * inst, int32_t anOpNumber, uint64_t val, std::list<InternalDependance> & dependances);
void addAnnulment( SemanticInstruction * inst, predicated_action & exec, InternalDependance const & aWritebackDependance);
void addRD1Annulment( SemanticInstruction * inst, predicated_action & exec, InternalDependance const & aWritebackDependance);
void addWriteback( SemanticInstruction * inst, eOperandCode aRegisterCode,eOperandCode aMappedRegisterCode, predicated_action & exec, bool a64, bool setflags, bool addSquash = true);

// Destination
void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool is64, bool setflags = false, bool addSquash = true);
void addPairDestination( SemanticInstruction * inst, uint32_t rd, uint32_t rd1, predicated_action & exec, bool is64, bool addSquash = true);
void addAddressCompute( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps);
void setRD( SemanticInstruction * inst, uint32_t rd);
void setRD1( SemanticInstruction * inst, uint32_t rd);
void setRS( SemanticInstruction * inst, eOperandCode rs_code, uint32_t rs);

// aux
predicated_action addExecute( SemanticInstruction * inst, std::unique_ptr<Operation> anOperation, std::vector< std::list<InternalDependance> > & rs_deps, eOperandCode aResult = kResult, boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));
predicated_action addExecute( SemanticInstruction * inst, std::unique_ptr<Operation> anOperation, eOperandCode anOperand1, eOperandCode anOperand2, std::vector< std::list<InternalDependance> > & rs_deps, eOperandCode aResult = kResult, boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));

void MEMBAR( SemanticInstruction * inst, uint32_t anAccess);
void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances);

uint64_t bitmask64(unsigned int length);
uint64_t bitfield_replicate(uint64_t mask, unsigned int e);

uint32_t highestSetBit(bits val);
uint64_t ones(uint64_t length);
uint64_t ror(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t lsl(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t lsr(uint64_t input, uint64_t input_size, uint64_t shift_size);
uint64_t asr(uint64_t input, uint64_t input_size, uint64_t shift_size);

arminst generateException(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, eExceptionType aType);

bool decodeBitMasks(uint64_t & tmask, uint64_t & wmask, bool immN, char imms, char immr, bool immediate, uint32_t dataSize);
bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc);
} // narmDecoder

#endif // FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED
