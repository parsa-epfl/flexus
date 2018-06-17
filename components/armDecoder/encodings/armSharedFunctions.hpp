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

namespace narmDecoder {
using namespace nuArchARM;

// Source
void addReadRD( SemanticInstruction * inst, uint32_t rd) ;
void addReadFDs( SemanticInstruction * inst, uint32_t fd, eSize aSize, std::vector<InternalDependance> & aValueDependance);
void addReadFValue( SemanticInstruction * inst, uint32_t fd, eSize aSize);
void addReadCC( SemanticInstruction * inst, int32_t ccNum, int32_t anOpNumber, std::list<InternalDependance> & dependances);
void addReadXRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances, bool is_64 = true);
void addAnnulment( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, InternalDependance const & aWritebackDependance);
void addWriteback( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, bool addSquash = true);

// Destination
void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool addSquash = true);
void addFloatingDestination( SemanticInstruction * inst, uint32_t fd, eSize aSize, predicated_action & exec);
void addAddressCompute( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps);

// aux
predicated_action addExecute_XTRA( SemanticInstruction * inst, Operation & anOperation, uint32_t rd, std::vector< std::list<InternalDependance> > & rs_deps, bool write_xtra);
predicated_action addExecute( SemanticInstruction * inst, Operation & anOperation, std::vector< std::list<InternalDependance> > & rs_deps);

void  MEMBAR( SemanticInstruction * inst, armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, uint32_t i );
void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances);

uint64_t bitmask64(unsigned int length);
uint64_t bitfield_replicate(uint64_t mask, unsigned int e);

bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc);
bool logic_imm_decode_wmask_tmask(uint64_t *wmask, uint64_t *tmask, unsigned int immn,  unsigned int imms, unsigned int immr);
bool logic_imm_decode_wmask(uint64_t *wmask, unsigned int immn, unsigned int imms, unsigned int immr);
} // narmDecoder

#endif // FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED
