#include <vector>

#include <core/simulator_layout.hpp>

#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT PowerTracker
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Frequency, double, "Clock frequency", "frq", 3e9)
  PARAMETER( NumL2Tiles, uint32_t, "Number of L2 cache tiles", "numL2Tiles", 1)

  PARAMETER( DecodeWidth, uint32_t, "Decode width", "decodeWidth", 4)
  PARAMETER( IssueWidth, uint32_t, "Issue width", "issueWidth", 4)
  PARAMETER( CommitWidth, uint32_t, "Commit width", "commitWidth", 4)
  PARAMETER( WindowSize, uint32_t, "Instruction window size", "windowSize", 64)
  PARAMETER( NumIntPhysicalRegisters, uint32_t, "Number of integer physical registers", "numIntPregs", 128)
  PARAMETER( NumFpPhysicalRegisters, uint32_t, "Number of floating point physical registers", "numFpPregs", 128)
  PARAMETER( NumIntArchitecturalRegs, uint32_t, "Number of integer architectural registers", "numIntAregs", 32)
  PARAMETER( NumFpArchitecturalRegs, uint32_t, "Number of floating point architectural registers", "numFpAregs", 32)

  PARAMETER( LsqSize, uint32_t, "Load/store queue size", "lsqSize", 64)
  PARAMETER( DataPathWidth, uint32_t, "Datapath width in bits", "dataPathWidth", 64)
  PARAMETER( InstructionLength, uint32_t, "Instruction length in bits", "instructionLength", 32)
  PARAMETER( VirtualAddressSize, uint32_t, "Virtual address length in bits", "virtualAddressSize", 64)
  PARAMETER( NumIntAlu, uint32_t, "Number of integer ALUs", "numIntAlu", 4)
  PARAMETER( NumIntMult, uint32_t, "Number of integer multipliers", "numIntMult", 1)
  PARAMETER( NumFpAdd, uint32_t, "Number of floating point ADD/SUB units", "numFpAdd", 2)
  PARAMETER( NumFpMult, uint32_t, "Number of floating point MUL/DIV units", "numFpMult", 1)
  PARAMETER( NumMemPort, uint32_t, "Number of memory access ports", "numMemPort", 2)

  // Branch predictor parameters
  PARAMETER( BtbNumSets, uint32_t, "Number of sets in branch target buffer", "btbNumSets", 2048)
  PARAMETER( BtbAssociativity, uint32_t, "Associativity of branch target buffer", "btbAssociativity", 16)
  PARAMETER( GShareSize, uint32_t, "Width of gshare shift register", "gShareSize", 13)
  PARAMETER( BimodalTableSize, uint32_t, "Table size for bimodal predictor", "bimodalTableSize", 16384)
  PARAMETER( MetaTableSize, uint32_t, "Metapredictor table size", "metaTableSize", 16384)
  PARAMETER( RasSize, uint32_t, "Return address stack size", "rasSize", 8)

  // Cache parameters
  PARAMETER( L1iNumSets, uint32_t, "Number of sets in L1 instruction cache", "l1iNumSets", 512)
  PARAMETER( L1iAssociativity, uint32_t, "Associativity of L1 instruction cache", "l1iAssociativity", 2)
  PARAMETER( L1iBlockSize, uint32_t, "Block size of L1 instruction cache in bytes", "l1iBlockSize", 64)
  PARAMETER( L1dNumSets, uint32_t, "Number of sets in L1 data cache", "l1dNumSets", 512)
  PARAMETER( L1dAssociativity, uint32_t, "Associativity of L1 data cache", "l1dAssociativity", 2)
  PARAMETER( L1dBlockSize, uint32_t, "Block size of L1 data cache in bytes", "l1dBlockSize", 64)
  PARAMETER( L2uNumSets, uint32_t, "Number of sets in one L2 unified cache tile", "l2uNumSets", 4096)
  PARAMETER( L2uAssociativity, uint32_t, "Associativity of L2 unified cache", "l2uAssociativity", 8)
  PARAMETER( L2uBlockSize, uint32_t, "Block size of L2 unified cache in bytes", "l2uBlockSize", 128)

  // TLB parameters
  PARAMETER( ItlbNumSets, uint32_t, "Number of sets in instruction TLB", "itlbNumSets", 1)
  PARAMETER( ItlbAssociativity, uint32_t, "Associativity of instruction TLB", "itlbAssociativity", 128)
  PARAMETER( ItlbPageSize, uint32_t, "Page size of instruction pages", "itlbPageSize", 4096)
  PARAMETER( DtlbNumSets, uint32_t, "Number of sets in data TLB", "dtlbNumSets", 1)
  PARAMETER( DtlbAssociativity, uint32_t, "Associativity of data TLB", "dtlbAssociativity", 128)
  PARAMETER( DtlbPageSize, uint32_t, "Page size of data pages", "dtlbPageSize", 4096)

  // Thermal model
  PARAMETER( HotSpotChipThickness, double, "Chip thickness in meters", "hotSpotChipThickness", 0.00015)
  PARAMETER( HotSpotConvectionCapacitance, double, "Convection capacitance in J/K", "hotSpotConvectionCapacitance", 140.4)
  PARAMETER( HotSpotConvectionResistance, double, "Convection resistance in K/W", "hotSpotConvectionResistance", 0.1)
  PARAMETER( HotSpotHeatsinkSide, double, "Heatsink side in meters", "hotSpotHeatsinkSide", 0.06)
  PARAMETER( HotSpotHeatsinkThickness, double, "Heatsink thickness in meters", "hotSpotHeatsinkThickness", 0.0069)
  PARAMETER( HotSpotHeatSpreaderSide, double, "Heat spreader side in meters", "hotSpotHeatSpreadeSide", 0.03)
  PARAMETER( HotSpotHeatSpreaderThickness, double, "Heat spreader thickness in meters", "hotSpotHeatSpreadeThickness", 0.001)
  PARAMETER( HotSpotInterfaceMaterialThickness, double, "Thermal interface material thickness in meters", "hotSpotChipInterfaceMaterialThickness", 2e-5)
  PARAMETER( HotSpotAmbientTemperature, double, "Celsius ambient temperature", "hotSpotAmbientTemperature", 45)
  PARAMETER( HotSpotFloorpanFile, std::string, "HotSpot floorplan file", "hotSpotFloorplanFile", "")
  PARAMETER( HotSpotInitialTemperatureFile, std::string, "HotSpot initial temperature input file", "hotSpotInitialTemperatureFile", "")
  PARAMETER( HotSpotSteadyStateTemperatureFile, std::string, "HotSpot steady-state temperature output file", "hotSpotSteadyStateTemperatureFile", "")
  PARAMETER( HotSpotInterval, uint32_t, "Number of cycles between temperature updates", "hotSpotInterval", 10000)
  PARAMETER( HotSpotResetCycle, uint32_t, "Reset HotSpot power and cycle counts after this many cycles to ensure steady-state temperatures are not computed using warm-up period", "hotSpotResetCycle", 0)

  PARAMETER( EnablePowerTracker, bool, "Whether PowerTracker is enabled", "enablePowerTracker", false)
  PARAMETER( EnableTemperatureTracker, bool, "Whether TemperatureTracker is enabled", "enableTemperatureTracker", false)
  PARAMETER( DefaultTemperature, double, "Celsius temperature used for all blocks if TemperatureTracker is disabled", "defaultTemperature", 100)
  PARAMETER( PowerStatInterval, uint32_t, "Power stats are only updated every this many baseline cycles. Aggregation reduces accuracy impact of not having a double StatCounter", "powerStatInterval", 50000)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, bool, L1iRequestIn)
  DYNAMIC_PORT_ARRAY( PushInput, long, DispatchedInstructionIn)
  DYNAMIC_PORT_ARRAY( PushInput, bool, L1dRequestIn)
  DYNAMIC_PORT_ARRAY( PushInput, bool, L2uRequestIn)
  DYNAMIC_PORT_ARRAY( PushInput, bool, StoreForwardingHitIn)
  DYNAMIC_PORT_ARRAY( PushInput, bool, CoreClockTickIn)
  DYNAMIC_PORT_ARRAY( PushInput, bool, L2ClockTickIn)

  // These interfaces could be useful if one is making decisions based on runtime power or temperature measurements
  DYNAMIC_PORT_ARRAY( PullOutput, double, CoreAveragePowerOut) // Average power used by core since last check
  DYNAMIC_PORT_ARRAY( PullOutput, double, L2AveragePowerOut)  // Average power used by L2 tile since last check
  DYNAMIC_PORT_ARRAY( PullOutput, double, CoreTemperatureOut)
  DYNAMIC_PORT_ARRAY( PullOutput, double, L2TemperatureOut)

  DRIVE( PowerTrackerDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PowerTracker
