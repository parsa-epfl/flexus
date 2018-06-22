#include <boost/throw_exception.hpp>
#include <memory>
#include <functional>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/qemu/api_wrappers.hpp>

#include "mai_api.hpp"

#include <core/qemu/ARMmmu.hpp>

#include <stdio.h>

#define DBG_DefineCategories MMUCat
#define DBG_SetDefaultOps AddCat(MMUCat)
#include DBG_Control()

namespace Flexus {
namespace Qemu {

#if FLEXUS_TARGET_IS(ARM)
using namespace Flexus::Qemu::API;

bool theMMUs_initialized = false;
std::vector<MMU::mmu_t> theMMUs;
std::vector<std::deque<MMU::mmu_t> > theMMUckpts;

std::vector<int> theMMUMap;

void armProcessorImpl::initializeMMUs() {
  if (!theMMUs_initialized) {
    theMMUs_initialized = true;

    int num_procs = ProcessorMapper::numProcessors();
    theMMUs.resize( num_procs );
    theMMUckpts.resize( num_procs );

    theMMUMap.resize( num_procs );

    for (unsigned int i = 0; i < theMMUs.size(); ++i) {
      API::conf_object_t * cpu = Qemu::API::QEMU_get_cpu_by_index(ProcessorMapper::mapFlexusIndex2ProcNum(i));
      //MMU::fm_init_mmu_from_simics( &theMMUs[i], SIM_get_attribute(cpu, "mmu").u.object );
      //ALEX - FIXME: Get mmu from QEMU
    }
  }
}

bool isTranslatingASI(int anASI) {
  switch (anASI) {
    case 0:
      return true;
    default:
      return false;
  }
}

long /*opcode*/ armProcessorImpl::fetchInstruction(Translation & aTranslation, bool aTakeTrap) {
  return fetchInstruction_MMUImpl(aTranslation, aTakeTrap);
}

void armProcessorImpl::translate(Translation & aTranslation, bool aTakeException) const {
  DBG_( Tmp, ( << "MMU: translating... " << VirtualMemoryAddress(aTranslation.theVaddr)) );
  translate_MMUImpl(aTranslation, aTakeException);
  DBG_( Tmp, ( << "MMU after translate " << PhysicalMemoryAddress(aTranslation.thePaddr)) );
}

unsigned long long armProcessorImpl::readVAddr(VirtualMemoryAddress anAddress, int anASI, int aSize) const {
  return readVAddr_QemuImpl(anAddress, anASI, aSize);
}

unsigned long long armProcessorImpl::readVAddrXendian(Translation & aTranslation, int aSize) const {
  return readVAddrXendian_MMUImpl(aTranslation, aSize);
}

PhysicalMemoryAddress armProcessorImpl::translateInstruction_QemuImpl( VirtualMemoryAddress anAddress) const {
  try {
    API::logical_address_t addr(anAddress);
    API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction, addr);
    //checkException();

    return PhysicalMemoryAddress(phy_addr);
  } catch (MemoryException & anError ) {

        DBG_(Tmp, (<<"ARM MMU: Got an Error: " << anError.what()));

    return PhysicalMemoryAddress(0);
  }
}

long armProcessorImpl::fetchInstruction_QemuImpl(VirtualMemoryAddress const & anAddress) {
  API::logical_address_t addr(anAddress);
  API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction, addr);
  checkException();

  long op_code = Qemu::API::QEMU_read_phys_memory( *this, phy_addr, 4);
  checkException();

  return op_code;
}

bool cacheable(API::arm_memory_transaction_t & xact) {
  return (xact.cache_virtual && xact.cache_physical );
}

bool side_effect(API::arm_memory_transaction_t & xact) {
  return (xact.side_effect || xact.s.inverse_endian);
}

std::tuple<PhysicalMemoryAddress, bool, bool> armProcessorImpl::translateTSB_QemuImpl(VirtualMemoryAddress anAddress, int anASI) const {
  //Check for known special-case ASIs

  try {
    API::arm_memory_transaction_t xact;
    translate_QemuImpl( xact, anAddress, anASI);
    return std::make_tuple( PhysicalMemoryAddress( xact.s.physical_address ), (xact.s.physical_address != 0 ) && cacheable(xact), (xact.s.physical_address == 0 ) || side_effect(xact)  );
  } catch (MemoryException & anError ) {
    return std::make_tuple( PhysicalMemoryAddress(0), false, true);
  }
}

unsigned long long endianFlip(unsigned long long val, int aSize) {
  unsigned long long ret_val = 0;
  switch (aSize) {
    case 1:
      ret_val = val;
      break;
    case 2:
      ret_val = ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);
      break;
    case 4:
      ret_val = ((val & 0xFF000000) >> 24) | ((val & 0xFF0000) >> 8) | ((val & 0xFF00) << 8) | ((val & 0xFF) << 24);
      break;
    case 8:
      ret_val =
        ((val & 0xFF00000000000000ULL) >> 56) | ((val & 0xFF000000000000ULL) >> 40) | ((val & 0xFF0000000000ULL) >> 24) | ((val & 0xFF00000000ULL) >> 8) |
        ((val & 0xFFULL) << 56)               | ((val & 0xFF00ULL) << 40)           | ((val & 0xFF0000ULL) << 24)      | ((val & 0xFF000000ULL) << 8)   ;
      break;
    default:
      DBG_Assert( false, ( << "Unsupported size in endian-flip: " << aSize) );
  }
  return ret_val;
}

unsigned long long armProcessorImpl::readVAddr_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const {
  try {
    API::arm_memory_transaction_t xact;
    translate_QemuImpl( xact, anAddress, anASI );
    unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, xact.s.physical_address, aSize);
    checkException();

    return value;
  } catch (MemoryException & anError ) {
    return 0;
  }
}

unsigned long long armProcessorImpl::readVAddrXendian_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const {

  try {
    API::arm_memory_transaction_t xact;
    translate_QemuImpl( xact, anAddress, anASI);

    DBG_(VVerb, ( << "Virtual: " << anAddress << " ASI: " << anASI << " Size: " << aSize << " Physical: " << xact.s.physical_address) );

    unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, xact.s.physical_address, aSize);
    checkException();

    if (xact.s.inverse_endian) {
      DBG_(Verb, ( << "Inverse endian access to " << anAddress << " ASI: " << anASI << " Size: " << aSize ) );
      value = endianFlip(value, aSize);
    }

    return value;
  } catch (MemoryException & anError ) {
    if (anASI == 0x80) {
      //Try again using the CPU to translate
      try {
        API::logical_address_t addr(anAddress);
        API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
        checkException();
        unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, phy_addr, aSize);
        checkException();
        return value;
      } catch (MemoryException & anError ) {}
    }
    return 0;
  }
}

void armProcessorImpl::translate_QemuImpl(  API::arm_memory_transaction_t & xact, VirtualMemoryAddress anAddress, int anASI ) const {

  if (! isTranslatingASI(anASI)) {
    throw MemoryException();
  }

  DBG_( Verb, ( << "Translating " << anAddress << " in ASI: " << std::hex << anASI << std::dec ) );
  API::logical_address_t addr(anAddress);
  memset( &xact, 0, sizeof(API::arm_memory_transaction_t ) );
  xact.priv = 1;
#if defined(CONFIG_QEMU)
  xact.access_type = API::ARM_Access_Normal;
#elif SIM_VERSION < 1200
  xact.align_kind = API::Align_Natural;
#else
  //align_kind was replaced by access_type in Simics 2.2.x
  xact.access_type = API::ARM_Access_Normal;
#endif
  xact.address_space = anASI;
  xact.s.logical_address = addr;
  xact.s.size = 4;
  xact.s.type = API::QEMU_Trans_Load;
  xact.s.inquiry = 1;
  xact.s.ini_type = API::QEMU_Initiator_Other;
  xact.s.exception = API::QEMU_PE_No_Exception;

  API::exception_type_t except;
  DBG_Assert(mmu());
  //except = mmu()->logical_to_physical( theMMU, &xact ) ;  //ALEX - FIXME: Need an MMU API in QEMU
  except = API::QEMU_PE_No_Exception;	//temp dummy

  if (anASI == 0x80) {
    //Translate via the CPU as well, to confirm that it gives the same paddr
    //as the MMU.  If it doesn't, override the MMU's answer with the CPU's
    //answer
    API::logical_address_t addr(anAddress);
    API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
    checkException();
    if (phy_addr != xact.s.physical_address ) {
      DBG_(Verb, ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] Translation difference between CPU and MMU for " << anAddress << ".  Using CPU translation of " << PhysicalMemoryAddress(phy_addr) ));
      xact.s.physical_address = phy_addr;
      return;
    }
  }

  if ( except != API::QEMU_PE_No_Exception ) {
    DBG_( Verb, ( << "Exception during translation: " << except ) );
    throw MemoryException();
  }

}

MMU::mmu_t armProcessorImpl::getMMU() {
  return theMMUs[id()];
}

void armProcessorImpl::ckptMMU() {
  DBG_(Verb, ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] checkpointing MMU. size=" << theMMUckpts[id()].size()));
  theMMUckpts[id()].push_back(theMMUs[id()]);
}

void armProcessorImpl::releaseMMUCkpt() {
  DBG_(Verb, ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] releasing oldest MMU checkpoint. size=" << theMMUckpts[id()].size()));
  DBG_Assert(!theMMUckpts[id()].empty(), ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] has no checkpoint to release"));
  theMMUckpts[id()].pop_front();
}

void armProcessorImpl::rollbackMMUCkpts(int n) {
  DBG_(Verb, ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] rolling back " << n << " MMU checkpoints"));
  DBG_Assert(theMMUckpts[id()].size() > (unsigned)n, ( << "CPU[" << Qemu::API::QEMU_get_processor_number(*this) << "] has " << theMMUckpts[id()].size() << " but needs > " << n));
  // remove n checkpoints to get back to where we started
  for (int i = 0; i < n; ++i) theMMUckpts[id()].pop_back();
  theMMUs[id()] = theMMUckpts[id()].back();
  theMMUckpts[id()].pop_back();
}

void armProcessorImpl::resyncMMU() {
  //MMU::fm_init_mmu_from_simics(&theMMUs[id()], SIM_get_attribute(*this, "mmu").u.object );	//ALEX - FIXME
}

bool armProcessorImpl::validateMMU(MMU::mmu_t * anMMU) {
  MMU::mmu_t simics_mmu, *our_mmu;
  //MMU::fm_init_mmu_from_simics(&simics_mmu, SIM_get_attribute(*this, "mmu").u.object);  //ALEX - FIXME
  assert(false);
  if (anMMU == NULL) our_mmu = &(theMMUs[id()]);
  else our_mmu = anMMU;
  if (MMU::fm_compare_mmus(our_mmu, &simics_mmu)) {
    return false;
  }
  return true;
}

unsigned long long armProcessorImpl::mmuRead(VirtualMemoryAddress anAddress, int anASI) {
  MMU::mmu_access_t access;
  access.va = anAddress;
  access.asi = anASI;
  access.type = MMU::mmu_access_load;
  mmu_access( &theMMUs[id()], & access );
  return access.val;
}

void armProcessorImpl::mmuWrite(VirtualMemoryAddress anAddress, int anASI, unsigned long long aValue) {
  MMU::mmu_access_t access;
  access.va = anAddress;
  access.asi = anASI;
  access.type = MMU::mmu_access_store;
  access.val = aValue;
  mmu_access( &theMMUs[id()], & access );
}

void armProcessorImpl::dumpMMU(MMU::mmu_t * anMMU) {
  MMU::mmu_t * m = (anMMU == NULL) ? &(theMMUs[id()]) : anMMU;
  MMU::fm_print_mmu_regs(m);
}

void armProcessorImpl::translate_MMUImpl(Translation & aTranslation, bool aTakeException) const {

    aTranslation.thePaddr = translateInstruction_QemuImpl(aTranslation.theVaddr);
    DBG_( Tmp, ( <<"\e[33m" << "MMU: " << VirtualMemoryAddress(aTranslation.theVaddr)<< " -> "
    << PhysicalMemoryAddress(aTranslation.thePaddr)<< "\e[0m")  );
}

unsigned long int Endian_DWord_Conversion(unsigned long int dword)//NOOSHIN
{
  //return ((dword>>24)&0x000000FF) | ((dword>>8)&0x0000FF00) | ((dword<<8)&0x00FF0000) | ((dword<<24)&0xFF000000);
    unsigned long int w = __builtin_bswap32 (dword);
    unsigned long int dw = __builtin_bswap64 (dword);
    return __builtin_bswap64 (dword);
}


long armProcessorImpl::fetchInstruction_MMUImpl(Translation & aTranslation, bool aTakeTrap) {
  uint64_t op_code = 0;

  translate_MMUImpl(aTranslation, aTakeTrap );
  if (aTranslation.thePaddr != 0) {
    try {
      op_code = Qemu::API::QEMU_read_phys_memory( *this, aTranslation.thePaddr, 4);
      //op_code = Endian_DWord_Conversion(op_code);
      checkException();
    } catch (...) {

      DBG_(Tmp,(<<"OPCODE IS GOING TO SET ZEROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO") );
      op_code = 0;
    }
  }

  DBG_(Tmp, (<< "\e[34m" <<"MMU: Opcode is "<<  std::hex << std::setw(8) << op_code << std::dec << "\e[0m"));
  return op_code;
}

unsigned long long armProcessorImpl::readVAddr_MMUImpl(Translation & aTranslation, int aSize) const {
  try {
    translate_MMUImpl(aTranslation, false);

    DBG_(Tmp, ( << "Virtual: " << aTranslation.theVaddr << " ASI: " << aTranslation.theASI << " Size: " << aSize << " Physical: " << aTranslation.thePaddr ) );//NOOSHIN

    unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, aTranslation.thePaddr, aSize);
    checkException();

    return value;
  } catch (MemoryException & anError ) { }
  return 0;
}

unsigned long long armProcessorImpl::readVAddrXendian_MMUImpl(Translation & aTranslation, int aSize) const {
  try {
    translate_MMUImpl(aTranslation, false);

    DBG_(VVerb, ( << "Virtual: " << aTranslation.theVaddr << " ASI: " << aTranslation.theASI << " Size: " << aSize << " Physical: " << aTranslation.thePaddr ) );

    unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, aTranslation.thePaddr, aSize);
    checkException();

    if (aTranslation.isXEndian()) {
      DBG_(Verb, ( << "Inverse endian access to " << aTranslation.theVaddr  << " ASI: " << aTranslation.theASI  << " Size: " << aSize ) );
      value = endianFlip(value, aSize);
    }

    if (aTranslation.theASI == 0x80 && (aTranslation.thePSTATE & 0x100 /*PSTATE.CLE*/)) {
      DBG_(Verb, ( << "PSTATE.CLE set.  Inverting endianness of  " << aTranslation.theVaddr  << " ASI: " << aTranslation.theASI  << " Size: " << aSize ) );
      value = endianFlip(value, aSize);
    }

    return value;
  } catch (MemoryException & anError ) { }
  return 0;
}

bool Translation::isCacheable() {
  return MMU::mmu_is_cacheable( theTTEEntry );
}
bool Translation::isSideEffect() {
  return MMU::mmu_is_sideeffect( theTTEEntry );
}
bool Translation::isXEndian() {
  return MMU::mmu_is_xendian( theTTEEntry );
}

bool Translation::isMMU() {
  switch (theASI) {
    case 0: //I-MMU
      return true;

    default: //all others
      return false;
  }
}
bool Translation::isInterrupt() {
  switch (theASI) {
    case 0:
      return true;

    default: //all others
      return false;
  }
}

bool Translation::isTranslating() {
  return isTranslatingASI(theASI);
}

namespace MMU {

using namespace Flexus::Qemu::API;

#define U_BIT_MASK (1ULL<<43)

static unsigned long long page_mask[4] = {0x1FFF, 0xFFFF, 0x7FFFF, 0x3FFFFF};

address_t mmu_make_paddr(tte_data data, address_t va) {
  int size = (data >> 61ULL) & 3;
  address_t pa = data;
  pa &= ~0xFFFFF80000000000ULL; /* all things above PA */
  pa &= ~page_mask[size];
  pa |= (va & page_mask[size]);
  return pa;
}

#define W_BIT (1ULL<<1)
#define E_BIT (1ULL<<3)
#define CV_BIT (1ULL<<4)
#define CP_BIT (1ULL<<5)
#define L_BIT (1ULL<<6)
#define U_BIT (1ULL<<43)
#define IE_BIT (1ULL<<59)
#define V_BIT (1ULL<<63)

#define CTXT_MASK 0x1FFF

bool mmu_is_cacheable(tte_data data) {
  return (!! (data & CV_BIT)) && (!! (data & CP_BIT));
}

bool mmu_is_sideeffect(tte_data data) {
  return !!(data & E_BIT);
}

bool mmu_is_xendian(tte_data data) {
  return !!(data & IE_BIT);
}

bool mmu_is_writeable(tte_data data) {
  return !!(data & W_BIT);
}

address_t
mmu_translate(mmu_t * mmu,
              unsigned int is_fetch,
              address_t va,
              unsigned int klass,
              unsigned int asi,
              unsigned int nofault,
              unsigned int priv,
              unsigned int access_type,
              mmu_exception_t * except,
              mmu_translation_type_t trans_type) {
  tte_data data = mmu_lookup(mmu, is_fetch, va, klass, asi, nofault, priv, access_type, except, trans_type);
  if (*except != no_exception) {
    return 0;
  }
  return mmu_make_paddr(data, va);
}

tte_data
mmu_lookup(mmu_t * mmu,
           unsigned int is_fetch,
           address_t va,
           unsigned int klass,
           unsigned int asi,
           unsigned int nofault,
           unsigned int priv,
           unsigned int access_type,
           mmu_exception_t * except,
           mmu_translation_type_t trans_type) {
  return (tte_data) 0;
}

address_t
mmu_generate_tsb_ptr(address_t va,
                     mmu_ptr_type_t type,
                     address_t tsb_base_in,
                     unsigned int tsb_split,
                     unsigned int tsb_size,
                     mmu_reg_t tsb_ext) {
  return (address_t) 0;
}

int mmu_fa_choose(mmu_t * mmu ) {
  return 0;
}

void
mmu_access(mmu_t * mmu, mmu_access_t * access) {
}

#define FM_GET_BITS(val, msb, lsb) ((val >> lsb) & ((1ULL<<(msb-lsb+1)) - 1))

void
fm_print_mmu_regs(mmu_t * mmu) {
  mmu_regs_t* mmu_regs = &(mmu->mmu_regs);
  // FIXME: print ARM control registers
}

//ALEX - FIXME: Pair the Flexus mmu with the QEMU mmu
/*
#define FM_COPY_FROM_SIMICS(ours, simics) \
do { attr_value_t val = SIM_get_attribute(chmmu, simics); \
     ours = val.u.integer; } while(0);

#define FM_COPY_LIST_FROM_SIMICS(ours, simics) \
do { attr_value_t val = SIM_get_attribute(chmmu, simics); \
     int i, size = val.u.list.size; \
     for (i = 0; i < size; i++) { \
       ours[i] = val.u.list.vector[i].u.integer; \
     } SIM_free_attribute(val); } while(0);
*/
/*
 * fm_init_mmu_from_simics: initializes an MMU to Simics MMU state
 */
/*
void
fm_init_mmu_from_simics(mmu_t * mmu, conf_object_t * chmmu) {
  tlb_regs_t * d_regs = &(mmu->d_regs);
  tlb_regs_t * i_regs = &(mmu->i_regs);

  FM_COPY_FROM_SIMICS(mmu->primary_context,      "ctxt_primary");
  FM_COPY_FROM_SIMICS(mmu->secondary_context,    "ctxt_secondary");

  FM_COPY_FROM_SIMICS(d_regs->sfar,              "dsfar");
  FM_COPY_FROM_SIMICS(d_regs->sfsr,              "dsfsr");
  FM_COPY_FROM_SIMICS(d_regs->tag_access,        "dtag_access");
  FM_COPY_FROM_SIMICS(d_regs->tsb_tag_target,    "dtag_target");
  FM_COPY_FROM_SIMICS(d_regs->tsb,               "dtsb");
  FM_COPY_FROM_SIMICS(d_regs->tsb_nx,            "dtsb_nx");
  FM_COPY_FROM_SIMICS(d_regs->tsb_px,            "dtsb_px");
  FM_COPY_FROM_SIMICS(d_regs->tsb_sx,            "dtsb_sx");
  FM_COPY_FROM_SIMICS(d_regs->tsbp64k,           "dtsbp64k");
  FM_COPY_FROM_SIMICS(d_regs->tsbp8k,            "dtsbp8k");
  FM_COPY_FROM_SIMICS(d_regs->tsbpd,             "dtsbpd");

  FM_COPY_FROM_SIMICS(i_regs->sfsr,              "isfsr");
  FM_COPY_FROM_SIMICS(i_regs->tag_access,        "itag_access");
  FM_COPY_FROM_SIMICS(i_regs->tsb_tag_target,    "itag_target");
  FM_COPY_FROM_SIMICS(i_regs->tsb,               "itsb");
  FM_COPY_FROM_SIMICS(i_regs->tsb_nx,            "itsb_nx");
  FM_COPY_FROM_SIMICS(i_regs->tsb_px,            "itsb_px");
  FM_COPY_FROM_SIMICS(i_regs->tsbp64k,           "itsbp64k");
  FM_COPY_FROM_SIMICS(i_regs->tsbp8k,            "itsbp8k");
  i_regs->sfar   = (mmu_reg_t)(-1);
  i_regs->tsb_sx = (mmu_reg_t)(-1);
  i_regs->tsbpd  = (mmu_reg_t)(-1);

  FM_COPY_FROM_SIMICS(mmu->lfsr,                 "lfsr");

  FM_COPY_FROM_SIMICS(mmu->pa_watchpoint,        "pa_watchpoint");
  FM_COPY_FROM_SIMICS(mmu->va_watchpoint,        "va_watchpoint");

  FM_COPY_LIST_FROM_SIMICS(mmu->dt16_tag,        "dtlb_fa_tagread");
  FM_COPY_LIST_FROM_SIMICS(mmu->dt16_data,       "dtlb_fa_daccess");
  FM_COPY_LIST_FROM_SIMICS(mmu->dt512_tag,       "dtlb_2w_tagread");
  FM_COPY_LIST_FROM_SIMICS(mmu->dt512_data,      "dtlb_2w_daccess");

  FM_COPY_LIST_FROM_SIMICS(mmu->it16_tag,        "itlb_fa_tagread");
  FM_COPY_LIST_FROM_SIMICS(mmu->it16_data,       "itlb_fa_daccess");
  FM_COPY_LIST_FROM_SIMICS(mmu->it128_tag,       "itlb_2w_tagread");
  FM_COPY_LIST_FROM_SIMICS(mmu->it128_data,      "itlb_2w_daccess");

}
*/
#define FM_COMPARE_FIELD(who, field) do { \
  if (a->field != b->field) { \
   DBG_(Crit, (<< "Mismatch in " << who << "->" << #field)); \
  DBG_(Crit, (<< " us:        " << std::hex << a->field)); \
  DBG_(Crit, (<< " simics:    " << std::hex << b->field)); \
    mismatch = 1; } } while(0);

#define FM_COMPARE_ARRAY(a, b, len, who) do { \
  int i; \
  for (i = 0; i < len; i++) { \
    if (a[i] != b[i]) { \
   DBG_(Crit, (<< "Mismatch in " << who << "[" << i << "]")); \
   DBG_(Crit, (<< " us:        " << std::hex << a[i])); \
   DBG_(Crit, (<< " simics:    " << std::hex << b[i])); \
      mismatch = 1; } } } while(0);

/*
 * fm_compare_regs: compare d- or i-tlb registers
 */
int
fm_compare_regs(mmu_regs_t* a, mmu_regs_t * b, const char * who) {
  return 0;
}

/* fm_compare_mmus: compares two MMUs and prints any differences */
int
fm_compare_mmus(mmu_t * a, mmu_t * b) {
  int mismatch = 0;
  mismatch |= fm_compare_regs(&(a->mmu_regs), &(b->mmu_regs), "mmu->d_regs");
  return mismatch;
}

bool is4KSupported() { return true; } // FIXME 
bool is16KGranuleSupported() { return false; } // FIXUS
bool is64KGranuleSupported() { return false; }

/* Msutherl - June'18
 * - Add code for TTE Descriptor classes
 */
address_t 
TTEDescriptor::extractBitRange(address_t input, unsigned upperBitBound, unsigned lowerBitBound)
{
    address_t result = input >> lowerBitBound;
    address_t upperMask = (1ULL << (upperBitBound-lowerBitBound+1))-1;
    return result & upperMask;
}

bool 
TTEDescriptor::extractSingleBitAsBool(address_t input, unsigned bitshift)
{
    address_t rawbit = ((input >> bitshift) & 0x1);
    return rawbit ? true : false ;
}

bool
TTEDescriptor::isValid() 
{
    return TTEDescriptor::extractSingleBitAsBool(rawDescriptor,0);
}

bool
TTEDescriptor::isTableEntry()
{
    return TTEDescriptor::extractSingleBitAsBool( rawDescriptor, 1 );
}

bool
TTEDescriptor::isBlockEntry()
{
    return !(TTEDescriptor::extractSingleBitAsBool( rawDescriptor, 1 ));
}

} // end namespace MMU
} //end Namespace Simics
} //end namespace Flexus

#endif // IS_ARM
