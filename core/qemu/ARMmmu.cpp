#include <boost/throw_exception.hpp>
#include <memory>
#include <functional>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/qemu/api_wrappers.hpp>

#include "mai_api.hpp"
#include <core/qemu/ARMmmu.hpp>
#include <stdio.h>
#include <core/MakeUniqueWrapper.hpp>

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

bool isTranslatingASI(int anASI) {
  switch (anASI) {
    case 0:
      return true;
    default:
      return false;
  }
}
unsigned long long armProcessorImpl::readVAddr(VirtualMemoryAddress anAddress, int anASI, int aSize) const {
  return readVAddr_QemuImpl(anAddress, anASI, aSize);
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
  //DBG_Assert(mmu());
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


unsigned long int Endian_DWord_Conversion(unsigned long int dword)//NOOSHIN
{
    return __builtin_bswap64 (dword);
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

namespace MMU {
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

/* Msutherl - June'18
 * - Add code for TTE Descriptor classes
 */
bool
TTEDescriptor::isValid() 
{
    return extractSingleBitAsBool(rawDescriptor,0);
}

bool
TTEDescriptor::isTableEntry()
{
    return extractSingleBitAsBool( rawDescriptor, 1 );
}

bool
TTEDescriptor::isBlockEntry()
{
    return !(extractSingleBitAsBool( rawDescriptor, 1 ));
}



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
fm_print_mmu_regs(mmu_regs_t* r) {
  std::cout << "SCTLR_EL1: " << std::hex << r->SCTLR[EL1] << std::dec << std::endl
            << "SCTLR_EL2: " << std::hex << r->SCTLR[EL2] << std::dec << std::endl
            << "SCTLR_EL3: " << std::hex << r->SCTLR[EL3] << std::dec << std::endl
            << "TCR_EL1: " << std::hex << r->TCR[EL1] << std::dec << std::endl
            << "TCR_EL2: " << std::hex << r->TCR[EL2] << std::dec << std::endl
            << "TCR_EL3: " << std::hex << r->TCR[EL3] << std::dec << std::endl
            << "TTBR0_EL1: " << std::hex << r->TTBR0[EL1] << std::dec << std::endl
            << "TTBR1_EL1: " << std::hex << r->TTBR1[EL1] << std::dec << std::endl
            << "TTBR0_EL2: " << std::hex << r->TTBR0[EL2] << std::dec << std::endl
            << "TTBR1_EL2: " << std::hex << r->TTBR1[EL2] << std::dec << std::endl
            << "TTBR0_EL3: " << std::hex << r->TTBR0[EL3] << std::dec << std::endl
            << "ID_AA64MMFR0_EL1: " << std::hex << r->ID_AA64MMFR0_EL1 << std::dec << std::endl;
            ;
}

void
mmu_t::setupBitConfigs() {
    // enable and endian-ness
    // - in SCTLR_ELx
    aarch64_bit_configs.M_Bit = 0; // 0b1 = MMU enabled
    aarch64_bit_configs.EE_Bit = 25; // endianness of EL1, 0b0 is little-endian
    aarch64_bit_configs.EoE_Bit = 24; // endianness of EL0, 0b0 is little-endian

    // Granule that this MMU supports
    // - in ID_AA64MMFR0_EL1
    aarch64_bit_configs.TGran4_Base = 28;  // = 0b0000 IF supported, = 0b1111 if not
    aarch64_bit_configs.TGran16_Base = 20; // = 0b0000 if NOT supported, 0b0001 IF supported (yes, this is correct)
    aarch64_bit_configs.TGran64_Base = 24; // = 0b0000 IF supported, = 0b1111 if not
    aarch64_bit_configs.TGran_NumBits = 4;

    // Granule configured for EL1 (TODO: add others if we ever care about them in the future)
    // - in TCR_ELx
    aarch64_bit_configs.TG0_Base = 14; /* 00 = 4KB
                              01 = 64KB
                              11 = 16KB */
    aarch64_bit_configs.TG0_NumBits = 2;
    aarch64_bit_configs.TG1_Base = 30; /* 01 = 16KB
                              10 = 4KB
                              11 = 64KB (yes, this is different than TG0) */
    aarch64_bit_configs.TG1_NumBits = 2;

    // Physical, output, and input address sizes. ( in ID_AA64MMFR0_EL1 )
    aarch64_bit_configs.PARange_Base = 0; /* 0b0000 = 32b
                                 0b0001 = 36b
                                 0b0010 = 40b
                                 0b0011 = 42b
                                 0b0100 = 44b
                                 0b0101 = 48b
                                 0b0110 = 52b */
    aarch64_bit_configs.PARange_NumBits = 4;

    // translation range sizes, chooses which VA's map to TTBR0 and TTBR1
    // - in TCR_ELx
    aarch64_bit_configs.TG0_SZ_Base = 0;
    aarch64_bit_configs.TG1_SZ_Base = 16;
    aarch64_bit_configs.TGn_SZ_NumBits = 6;
}

void
mmu_t::initRegsFromQEMUObject(mmu_regs_t* qemuRegs)
{
    setupBitConfigs(); // initialize AARCH64 bit locations for masking

    mmu_regs.SCTLR[EL1] = qemuRegs->SCTLR[EL1];
    mmu_regs.SCTLR[EL2] = qemuRegs->SCTLR[EL2];
    mmu_regs.SCTLR[EL3] = qemuRegs->SCTLR[EL3];
    mmu_regs.TCR[EL1] = qemuRegs->TCR[EL1];
    mmu_regs.TCR[EL2] = qemuRegs->TCR[EL2];
    mmu_regs.TCR[EL3] = qemuRegs->TCR[EL3];
    mmu_regs.TTBR0[EL1] = qemuRegs->TTBR0[EL1];
    mmu_regs.TTBR1[EL1] = qemuRegs->TTBR1[EL1];
    mmu_regs.TTBR0[EL2] = qemuRegs->TTBR0[EL2];
    mmu_regs.TTBR1[EL2] = qemuRegs->TTBR1[EL2];
    mmu_regs.TTBR0[EL3] = qemuRegs->TTBR0[EL3];
    mmu_regs.ID_AA64MMFR0_EL1 = qemuRegs->ID_AA64MMFR0_EL1;
    DBG_(Tmp , ( << "Initializing mmu registers from QEMU...."));
    fm_print_mmu_regs(&(this->mmu_regs));
}

bool
mmu_t::IsExcLevelEnabled(uint8_t EL) const {
    DBG_Assert( EL > 0 && EL <= 3,
                ( << "ERROR, ARM MMU: Transl. Request Not Supported at Invalid EL = " << EL ) );
    return extractSingleBitAsBool ( mmu_regs.SCTLR[EL], aarch64_bit_configs.M_Bit );
}

void
mmu_t::setupAddressSpaceSizesAndGranules(void)
{
    unsigned TG0_Size = getGranuleSize(0);
    unsigned TG1_Size = getGranuleSize(1);
    unsigned PASize = parsePASizeFromRegs();
    PASize = 48; // FIXME: ID_AAMMFR0 REGISTER ALWAYS 0 FROM QEMU????
    unsigned BR0_Offset = getIAOffsetValue(true);
    unsigned BR1_Offset = getIAOffsetValue(false);
    this->Gran0 = std::make_shared<TG0_Granule>(TG0_Size,PASize,BR0_Offset);
    this->Gran1 = std::make_shared<TG1_Granule>(TG1_Size,PASize,BR1_Offset);
}

/* Magic numbers taken from:
 * ARMv8-A ref manual, Page D7-2335
 */
bool 
mmu_t::is4KGranuleSupported() { 
    address_t TGran4 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                                            aarch64_bit_configs.TGran4_Base,
                                            aarch64_bit_configs.TGran_NumBits);
    return ( TGran4 == 0b0000 );
}
bool 
mmu_t::is16KGranuleSupported() {
    address_t TGran16 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                                            aarch64_bit_configs.TGran16_Base,
                                            aarch64_bit_configs.TGran_NumBits);
    return ( TGran16 == 0b0001 );
}
bool 
mmu_t::is64KGranuleSupported() { 
    address_t TGran64 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                                            aarch64_bit_configs.TGran64_Base,
                                            aarch64_bit_configs.TGran_NumBits);

    return ( TGran64 == 0b0000 );
}

/* Magic numbers taken from:
 * ARMv8-A ref manual, Page D7-2487.
 */
unsigned
mmu_t::getGranuleSize(unsigned granuleNum)
{
    if( granuleNum == 0 ) { // TG0
        address_t TG_SZ = extractBitsWithRange(mmu_regs.TCR[EL1],
                                                aarch64_bit_configs.TG0_Base,
                                                aarch64_bit_configs.TG0_NumBits);
        switch ( TG_SZ ) {
            case 0b00:
                return 1<<12; // 4KB
            case 0b01:
                return 1<<16; // 64KB
            case 0b11:
                return 1<<14; // 16KB
            default:
                DBG_Assert(false, ( << "Unknown value in getting TG0 Granule Size. TG_SZ = " << TG_SZ ));
                return 0;
        }
    } else { // TG1
        address_t TG_SZ = extractBitsWithRange(mmu_regs.TCR[EL1],
                                                aarch64_bit_configs.TG1_Base,
                                                aarch64_bit_configs.TG1_NumBits);
        switch ( TG_SZ ) {
            case 0b10:
                return 1<<12; // 4KB
            case 0b11:
                return 1<<16; // 64KB
            case 0b01:
                return 1<<14; // 16KB
            default:
                DBG_Assert(false, ( << "Unknown value in getting TG1 Granule Size. TG_SZ = " << TG_SZ ));
                return 0;
        }
    }
}

unsigned
mmu_t::parsePASizeFromRegs()
{
    address_t pRange_Config = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                                                    aarch64_bit_configs.PARange_Base,
                                                    aarch64_bit_configs.PARange_NumBits);
    /* Magic numbers taken from:
     * ARMv8-A ref manual, Page D4-2014.
     */
    switch( pRange_Config ) {
        case 0b0000:
            return 32;
        case 0b0001:
            return 36;
        case 0b0010:
            return 40;
        case 0b0011:
            return 42;
        case 0b0100:
            return 44;
        case 0b0101:
            return 48;
        case 0b0110:
            return 52;
        default:
            DBG_Assert(false, ( << "Unknown value in getting PAddr Width. Raw Value from MMU = " << pRange_Config ));
            return 0;
    }
}

unsigned
mmu_t::getIAOffsetValue(bool isBRO)
{
    address_t ret = 16;
    if( isBRO ) { // brooooooooooo, chahh!
        ret = extractBitsWithRange(mmu_regs.TCR[EL1],
                aarch64_bit_configs.TG0_SZ_Base,
                aarch64_bit_configs.TGn_SZ_NumBits);
    } else { // cheerio old boy!
        ret = extractBitsWithRange(mmu_regs.TCR[EL1],
                aarch64_bit_configs.TG1_SZ_Base,
                aarch64_bit_configs.TGn_SZ_NumBits);
    }
    return ret;
}

int
mmu_t::checkBR0RangeForVAddr( Translation& aTr ) const {
    int64_t upperBR0Bound = Gran0->GetUpperAddressRangeLimit();
    int64_t lowerBR1Bound = Gran1->GetLowerAddressRangeLimit();
    if( aTr.theVaddr <= upperBR0Bound ) {
        return 0; // br0
    } else if ( aTr.theVaddr >= lowerBR1Bound ) {
        return 1; // br1
    } else return -1; // fault
}

uint8_t 
mmu_t::getInitialLookupLevel( Translation& currentTr) const {
    return ( currentTr.isBR0 ? Gran0->getInitialLookupLevel()
            : Gran1->getInitialLookupLevel() );
}

uint8_t
mmu_t::getIASize(bool isBR0) const {
    return ( isBR0 ? Gran0->getIAddrSize() 
            : Gran1->getIAddrSize() );
}

uint8_t
mmu_t::getPAWidth(bool isBR0) const {
    return ( isBR0 ? Gran0->getPAddrWidth() 
            : Gran1->getPAddrWidth() );
}

} // end namespace MMU

} //end Namespace Simics
} //end namespace Flexus

#endif // IS_ARM
