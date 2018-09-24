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

typedef enum eConstraint
{// General:;LLLLLLLLLLLLLLLLLLL 
    kConstraint_NONE,
    kConstraint_UNKNOWN,
    kConstraint_UNDEF,
    kConstraint_NOP,
    kConstraint_TRUE,
    kConstraint_FALSE,
    kConstraint_DISABLED,
    kConstraint_UNCOND,
    kConstraint_COND,
    kConstraint_ADDITIONAL_DECODE,
    // Load-store:
    kConstraint_WBSUPPRESS,
    kConstraint_FAULT,
    // IPA too large
    kConstraint_FORCE,
    kConstraint_FORCENOSLCHECK
}eConstraint;

typedef enum eUnpredictable
{
    // Writeback/transfer register overlap (load):
    kUnpredictable_WBOVERLAPLD,
    // Writeback/transfer register overlap (store):
    kUnpredictable_WBOVERLAPST,
    // Load Pair transfer register overlap:
    kUnpredictable_LDPOVERLAP,
    // Store-exclusive base/status register overlap
    kUnpredictable_BASEOVERLAP,
    // Store-exclusive data/status register overlap
    kUnpredictable_DATAOVERLAP,
    // Load-store alignment checks:
    kUnpredictable_DEVPAGE2,
    // Instruction fetch from Device memory
    kUnpredictable_INSTRDEVICE,
    // Reserved MAIR value
    kUnpredictable_RESMAIR,
    // Reserved TEX:C:B value
    kUnpredictable_RESTEXCB,
    // Reserved PRRR value
    kUnpredictable_RESPRRR,
    // Reserved DACR field
    kUnpredictable_RESDACR,
    // Reserved VTCR.S value
    kUnpredictable_RESVTCRS,
    // Reserved TCR.TnSZ valu
    kUnpredictable_RESTnSZ,
    // IPA size exceeds PA size
    kUnpredictable_LARGEIPA,
    // Syndrome for a known-passing conditional A32 instruction
    kUnpredictable_ESRCONDPASS,
    // Illegal State exception: zero PSTATE.IT
    kUnpredictable_ILZEROIT,
    // Illegal State exception: zero PSTATE.T
    kUnpredictable_ILZEROT,
    // Debug: prioritization of Vector Catch
    kUnpredictable_BPVECTORCATCHPRI,
    // Debug Vector Catch: match on 2nd halfword
    kUnpredictable_VCMATCHHALF,
    // Debug Vector Catch: match on Data Abort or Prefetch abort
    kUnpredictable_VCMATCHDAPA,
    // Debug watchpoints: non-zero MASK and non-ones BAS
    kUnpredictable_WPMASKANDBAS,
    // Debug watchpoints: non-contiguous BAS
    kUnpredictable_WPBASCONTIGUOUS,
    // Debug watchpoints: reserved MASK
    kUnpredictable_RESWPMASK,
    // Debug watchpoints: non-zero MASKed bits of address
    kUnpredictable_WPMASKEDBITS,
    // Debug breakpoints and watchpoints: reserved control bits
    kUnpredictable_RESBPWPCTRL,
    // Debug breakpoints: not implemented
    kUnpredictable_BPNOTIMPL,
    // Debug breakpoints: reserved type
    kUnpredictable_RESBPTYPE,
    // Debug breakpoints: not-context-aware breakpoint
    kUnpredictable_BPNOTCTXCMP,
    // Debug breakpoints: match on 2nd halfword of instruction
    kUnpredictable_BPMATCHHALF,
    // Debug breakpoints: mismatch on 2nd halfword of instruction
    kUnpredictable_BPMISMATCHHALF,
    // Debug: restart to a misaligned AArch32 PC value
    kUnpredictable_RESTARTALIGNPC,
    // Debug: restart to a not-zero-extended AArch32 PC value
    kUnpredictable_RESTARTZEROUPPERPC,
    // Zero top 32 bits of X registers in AArch32 state:
    kUnpredictable_ZEROUPPER,
    // Zero top 32 bits of PC on illegal return to AArch32 state
    kUnpredictable_ERETZEROUPPERPC,
    // Force address to be aligned when interworking branch to A32 state
    kUnpredictable_A32FORCEALIGNPC,
    // SMC disabled
    kUnpredictable_SMD,
    // Access Flag Update by HW
    kUnpredictable_AFUPDATE,
    // Consider SCTLR[].IESB in Debug state
    kUnpredictable_IESBinDebug,
    // No events selected in PMSEVFR_EL1
    kUnpredictable_ZEROPMSEVFR,
    // No instruction type selected in PMSFCR_EL1
    kUnpredictable_NOINSTRTYPES,
    // Zero latency in PMSLATFR_EL1
    kUnpredictable_ZEROMINLATENCY,
    // To be determined -- THESE SHOULD BE RESOLVED (catch-all)
    kUnpredictable_TBD
}eUnpredictable;


eConstraint ConstrainUnpredictable(eUnpredictable which);

// Source
void addUpdateData( SemanticInstruction * inst, uint32_t data_reg, size_t size);
//void addReadRD( SemanticInstruction * inst, uint32_t rd) ;
void addReadFDs( SemanticInstruction * inst, uint32_t fd, eSize aSize, std::vector<InternalDependance> & aValueDependance);
void addReadFValue( SemanticInstruction * inst, uint32_t fd, eSize aSize);
void addReadCC( SemanticInstruction * inst, int32_t ccNum, int32_t anOpNumber, std::list<InternalDependance> & dependances);
void addReadXRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances, bool is_64 = true);
void addReadConstant (SemanticInstruction * inst, int32_t anOpNumber, int val, std::list<InternalDependance> & dependances);
void addReadVRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances);

void addCheckSystemAccess(SemanticInstruction * inst, uint32_t op0, uint32_t op1, uint32_t crn, uint32_t crm, uint32_t op2, uint32_t rt);

void addAnnulment( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, InternalDependance const & aWritebackDependance);
void addRD1Annulment( SemanticInstruction * inst, predicated_action & exec, InternalDependance const & aWritebackDependance);
void addWriteback( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, bool addSquash = true);
void addPrivWriteback( SemanticInstruction * inst, predicated_action & exec, uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm);
void addWriteback( SemanticInstruction * inst, eRegisterType aType, bool addSquash = true);
void addRD1Writeback( SemanticInstruction * inst, predicated_action & exec);

// Destination
void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool addSquash = true);
void addPairDestination( SemanticInstruction * inst, uint32_t rd, uint32_t rd1, predicated_action & exec, bool addSquash = true);
void addVDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool addSquash = true);
void addFloatingDestination( SemanticInstruction * inst, uint32_t fd, eSize aSize, predicated_action & exec);
void addAddressCompute( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps);
void setRD( SemanticInstruction * inst, uint32_t rd);

// aux
predicated_action addExecute_XTRA( SemanticInstruction * inst, std::unique_ptr<Operation> & anOperation, uint32_t rd, std::vector< std::list<InternalDependance> > & rs_deps, bool write_xtra);
predicated_action addExecute( SemanticInstruction * inst, std::unique_ptr<Operation> & anOperation, std::vector< std::list<InternalDependance> > & rs_deps, eOperandCode aResult = kResult, boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));
predicated_action addExecute2( SemanticInstruction * inst, std::unique_ptr<Operation> anOperation, std::vector< std::list<InternalDependance> > & rs_deps, eOperandCode aResult = kResult, boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none));

void  MEMBAR( SemanticInstruction * inst, armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, uint32_t i );
void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances);

uint64_t bitmask64(unsigned int length);
uint64_t bitfield_replicate(uint64_t mask, unsigned int e);

bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc);
bool logic_imm_decode_wmask_tmask(uint64_t *wmask, uint64_t *tmask, unsigned int immn,  unsigned int imms, unsigned int immr);
bool logic_imm_decode_wmask(uint64_t *wmask, unsigned int immn, unsigned int imms, unsigned int immr);
} // narmDecoder

#endif // FLEXUS_armDECODER_armSHAREDFUNCTIONS_HPP_INCLUDED
