#include <components/PowerTracker/PowerTracker.hpp>
#include <components/PowerTracker/CorePowerCosts.hpp>
#include <components/PowerTracker/L2PowerCosts.hpp>
#include <components/PowerTracker/PerBlockPower.hpp>
#include <components/PowerTracker/def.hpp>
#include <components/PowerTracker/TemperatureTracker.hpp>
#include <components/PowerTracker/WattchFunctions.hpp>
#include <core/stats.hpp>

#include <cmath>
#include <sstream>
#include <cstring>
#include <vector>

#define FLEXUS_BEGIN_COMPONENT PowerTracker
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories PowerTracker
#define DBG_SetDefaultOps AddCat(PowerTracker)
#include DBG_Control()

namespace nPowerTracker {

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(PowerTracker) {
  FLEXUS_COMPONENT_IMPL(PowerTracker);
private:
  // Struct to track number of accesses to various microarchitectural structures in each core
  struct CoreAccessCounts {
    uint32_t  l1iAccesses,
           intRenameAccesses,
           fpRenameAccesses,
           bpredAccesses,
           intRegFileAccesses,
           fpRegFileAccesses,
           resultBusAccesses,
           intAluAccesses,
           intRsAccesses,
           intSelectionAccesses,
           intWakeupAccesses,
           fpRsAccesses,
           fpSelectionAccesses,
           fpWakeupAccesses,
           fpAddAccesses,
           fpMulAccesses,
           memRsAccesses,
           memSelectionAccesses,
           memWakeupAccesses,
           lsqWakeupAccesses,
           lsqRsAccesses,
           l1dAccesses,
           clockTicks;

    CoreAccessCounts() :
      l1iAccesses(0),
      intRenameAccesses(0),
      fpRenameAccesses(0),
      bpredAccesses(0),
      intRegFileAccesses(0),
      fpRegFileAccesses(0),
      resultBusAccesses(0),
      intAluAccesses(0),
      intRsAccesses(0),
      intSelectionAccesses(0),
      intWakeupAccesses(0),
      fpRsAccesses(0),
      fpSelectionAccesses(0),
      fpWakeupAccesses(0),
      fpAddAccesses(0),
      fpMulAccesses(0),
      memRsAccesses(0),
      memSelectionAccesses(0),
      memWakeupAccesses(0),
      lsqWakeupAccesses(0),
      lsqRsAccesses(0),
      l1dAccesses(0),
      clockTicks(0)
    {}
  };

  // Struct to track scaling factor for various microarchitectural structures in each core to scale leakage power for temperature)
  struct CorePowerScalingFactors {
    double l1iFactor,
           itlbFactor,
           bpredFactor,
           intRenameFactor,
           fpRenameFactor,
           intRegfileFactor,
           fpRegfileFactor,
           windowFactor,
           intAluFactor,
           fpAddFactor,
           fpMulFactor,
           l1dFactor,
           dtlbFactor,
           lsqFactor,
           resultBusFactor;

    CorePowerScalingFactors() :
      l1iFactor(1),
      itlbFactor(1),
      bpredFactor(1),
      intRenameFactor(1),
      fpRenameFactor(1),
      intRegfileFactor(1),
      fpRegfileFactor(1),
      windowFactor(1),
      intAluFactor(1),
      fpAddFactor(1),
      fpMulFactor(1),
      l1dFactor(1),
      dtlbFactor(1),
      lsqFactor(1),
      resultBusFactor(1)
    {}
  };

  uint32_t numCoreTiles;
  uint32_t numL2Tiles;

  double vdd;

  // Indexed by core/L2 index
  std::vector<CoreAccessCounts> coreAccesses;
  std::vector<uint32_t> l2Accesses;
  std::vector<uint32_t> l2ClockTicks;

  // Indexed by core/L2 index
  std::vector<CorePowerScalingFactors> coreLeakageScalingFactors;
  std::vector<double> l2LeakageScalingFactors;

  CorePowerCosts coreAccessCosts;
  double l2AccessCost;
  double l2ClockCost;

  // Indexed by core/L2 index
  std::vector<double> lastCoreRequestPowers;
  std::vector<uint32_t> lastCoreRequestCycles;
  std::vector<double> lastL2RequestPowers;
  std::vector<uint32_t> lastL2RequestCycles;

  // Number of transistors for leakage model
  double N_INT_RENAME_DEVICES,
         N_FP_RENAME_DEVICES,
         N_BPRED_DEVICES,
         N_WINDOW_PREG_DEVICES,
         N_WINDOW_SELECT_DEVICES,
         N_WINDOW_WAKEUP_DEVICES,
         N_LSQ_PREG_DEVICES,
         N_LSQ_SELECT_DEVICES,
         N_LSQ_WAKEUP_DEVICES,
         N_INT_REGFILE_DEVICES,
         N_FP_REGFILE_DEVICES,
         N_ICACHE_DEVICES,
         N_DCACHE_DEVICES,
         N_UCACHE2_DEVICES,
         N_IALU_DEVICES,
         N_FALU_DEVICES,
         N_ITLB_DEVICES,
         N_DTLB_DEVICES;

  PerBlockPower powerUsed;
  PerBlockPower lastCheckpointPower;
  PerBlockPower lastHotSpotPower;

  TemperatureTracker temperatureTracker;

  uint32_t intervalCounter;
  uint32_t hotSpotCounter;

  Stat::StatCounter dynamic_Stat;
  Stat::StatCounter leakage_Stat;
  Stat::StatCounter total_Stat;

  // Not sure which of these two ways is most useful, so do both for now
  std::vector< Stat::StatCounter * > coreDynamic_Stats;
  std::vector< Stat::StatCounter * > coreLeakage_Stats;
  std::vector< Stat::StatCounter * > coreTotal_Stats;
  std::vector< Stat::StatCounter * > l2Dynamic_Stats;
  std::vector< Stat::StatCounter * > l2Leakage_Stats;
  std::vector< Stat::StatCounter * > l2Total_Stats;

  uint64_t totalBaselineCycles;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(PowerTracker)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ),
      vdd(VDD),
      dynamic_Stat(statName() + "-Dynamic"),
      leakage_Stat(statName() + "-Leakage"),
      total_Stat(statName() + "-Total")
  {    }

  void initialize() {
    uint32_t i;
    thermal_config_t hotSpotConfig;
    std::stringstream parseStream;

    DBG_Assert(!(!cfg.EnablePowerTracker && cfg.EnableTemperatureTracker), ( << "Cannot enable TemperatureTracker but not PowerTracker"));

    numCoreTiles = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    numL2Tiles = cfg.NumL2Tiles;

    intervalCounter = 1;
    hotSpotCounter = 1;

    coreAccesses.resize(numCoreTiles);
    l2Accesses.resize(numL2Tiles);
    l2ClockTicks.resize(numL2Tiles);

    coreLeakageScalingFactors.resize(numCoreTiles);
    l2LeakageScalingFactors.resize(numL2Tiles);

    lastCoreRequestPowers.resize(numCoreTiles, 0);
    lastCoreRequestCycles.resize(numCoreTiles, 0);
    lastL2RequestPowers.resize(numL2Tiles, 0);
    lastL2RequestCycles.resize(numL2Tiles, 0);

    // Set up stat arrays
    for (i = 0; i < numCoreTiles; ++i) {
      parseStream.str("");
      parseStream << statName() << "-Core-" << i << "-Dynamic";
      coreDynamic_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
      parseStream.str("");
      parseStream << statName() << "-Core-" << i << "-Leakage";
      coreLeakage_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
      parseStream.str("");
      parseStream << statName() << "-Core-" << i << "-Total";
      coreTotal_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
    }
    for (i = 0; i < numL2Tiles; ++i) {
      parseStream.str("");
      parseStream << statName() << "-L2-" << i << "-Dynamic";
      l2Dynamic_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
      parseStream.str("");
      parseStream << statName() << "-L2-" << i << "-Leakage";
      l2Leakage_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
      parseStream.str("");
      parseStream << statName() << "-L2-" << i << "-Total";
      l2Total_Stats.push_back(new Stat::StatCounter( parseStream.str() ));
    }

    powerUsed.setNumTiles(numCoreTiles, numL2Tiles);
    lastCheckpointPower.setNumTiles(numCoreTiles, numL2Tiles);
    lastHotSpotPower.setNumTiles(numCoreTiles, numL2Tiles);

    coreAccessCosts.setBaseVdd(vdd);
    coreAccessCosts.setVdd(vdd);
    coreAccessCosts.setFrq(cfg.Frequency);
    coreAccessCosts.setDecodeWidth(cfg.DecodeWidth);
    coreAccessCosts.setIssueWidth(cfg.IssueWidth);
    coreAccessCosts.setCommitWidth(cfg.CommitWidth);
    coreAccessCosts.setWindowSize(cfg.WindowSize);
    coreAccessCosts.setNumIntPhysicalRegisters(cfg.NumIntPhysicalRegisters);
    coreAccessCosts.setNumFpPhysicalRegisters(cfg.NumFpPhysicalRegisters);
    coreAccessCosts.setNumIntArchitecturalRegs(cfg.NumIntArchitecturalRegs);
    coreAccessCosts.setNumFpArchitecturalRegs(cfg.NumFpArchitecturalRegs);
    coreAccessCosts.setLsqSize(cfg.LsqSize);
    coreAccessCosts.setDataPathWidth(cfg.DataPathWidth);
    coreAccessCosts.setInstructionLength(cfg.InstructionLength);
    coreAccessCosts.setVirtualAddressSize(cfg.VirtualAddressSize);
    coreAccessCosts.setResIntAlu(cfg.NumIntAlu + cfg.NumIntMult);
    coreAccessCosts.setResFpAdd(cfg.NumFpAdd);
    coreAccessCosts.setResFpMult(cfg.NumFpMult);
    coreAccessCosts.setResMemPort(cfg.NumMemPort);
    coreAccessCosts.setBtbNumSets(cfg.BtbNumSets);
    coreAccessCosts.setBtbAssociativity(cfg.BtbAssociativity);
    coreAccessCosts.setTwoLevelL1Size(1); // Gshare
    coreAccessCosts.setTwoLevelL2Size(pow2(cfg.GShareSize));
    coreAccessCosts.setTwoLevelHistorySize(cfg.GShareSize);
    coreAccessCosts.setBimodalTableSize(cfg.BimodalTableSize);
    coreAccessCosts.setMetaTableSize(cfg.MetaTableSize);
    coreAccessCosts.setRasSize(cfg.RasSize);
    coreAccessCosts.setL1iNumSets(cfg.L1iNumSets);
    coreAccessCosts.setL1iAssociativity(cfg.L1iAssociativity);
    coreAccessCosts.setL1iBlockSize(cfg.L1iBlockSize);
    coreAccessCosts.setL1dNumSets(cfg.L1dNumSets);
    coreAccessCosts.setL1dAssociativity(cfg.L1dAssociativity);
    coreAccessCosts.setL1dBlockSize(cfg.L1dBlockSize);
    coreAccessCosts.setItlbNumSets(cfg.ItlbNumSets);
    coreAccessCosts.setItlbAssociativity(cfg.ItlbAssociativity);
    coreAccessCosts.setItlbPageSize(cfg.ItlbPageSize);
    coreAccessCosts.setDtlbNumSets(cfg.DtlbNumSets);
    coreAccessCosts.setDtlbAssociativity(cfg.DtlbAssociativity);
    coreAccessCosts.setDtlbPageSize(cfg.DtlbPageSize);
    coreAccessCosts.setSystemWidth(numCoreTiles);
    coreAccessCosts.computeAccessCosts();

    l2AccessCost = calculateL2AccessCost(cfg.Frequency, vdd, cfg.NumMemPort, cfg.L2uNumSets, cfg.L2uBlockSize, cfg.L2uAssociativity, cfg.VirtualAddressSize);
    l2ClockCost = perL2ClockPower(CORE_WIDTH, CORE_HEIGHT, DIE_WIDTH, DIE_HEIGHT, cfg.Frequency, vdd, numL2Tiles);

    // assume the Rename Table as RAM, log(N_PHYSICAL_REGS) x N_ARCHITECTED_REGS
    // Unused Rename Regs Fifo -> (N_PHYSICAL_REGS - N_ARCHITECTED_REGS) x log(N_PHYSICAL_REGS)
    // Separate integer and FP register files
    N_INT_RENAME_DEVICES = cfg.NumIntArchitecturalRegs * log2(cfg.NumIntPhysicalRegisters + cfg.NumFpPhysicalRegisters) * (K_RAM * 6) +
                           (cfg.NumIntPhysicalRegisters - cfg.NumIntArchitecturalRegs) * log2(cfg.NumIntPhysicalRegisters) * (K_D_FF * 22);

    N_FP_RENAME_DEVICES = cfg.NumFpArchitecturalRegs * log2(cfg.NumFpPhysicalRegisters) * (K_RAM * 6) +
                          (cfg.NumFpPhysicalRegisters - cfg.NumFpArchitecturalRegs) * log2(cfg.NumFpPhysicalRegisters) * (K_D_FF * 22);

    // Flexus uses gshare and a bimodal predictor with a metapredictor
    N_BPRED_DEVICES = cfg.BtbNumSets * cfg.BtbAssociativity * (64 - log2(cfg.BtbNumSets) + 64) * (K_RAM * 6)  + // BTB
                      cfg.GShareSize  * (K_D_FF * 22) + pow2(cfg.GShareSize) * 2 * (K_RAM * 6) + // GShare
                      cfg.BimodalTableSize * 2 * (K_RAM * 6) + // Bimodal
                      cfg.MetaTableSize * 2 * (K_RAM * 6) + // Metapredictor
                      cfg.RasSize * 64 * (K_D_FF * 22); // RAS

    // assume 64 bits per instruction, counters, tags, etc - aprox 128 bits per entry
    N_WINDOW_PREG_DEVICES = cfg.WindowSize * 128 * (K_D_LATCH * 10);

    // assume 8-input devices, on 3 levels, and issue width of 8
    N_WINDOW_SELECT_DEVICES = ((cfg.WindowSize - 1) / 7 * 111) * cfg.IssueWidth * (K_STATIC * 2); // From "Variability and Energy Awareness: A Microarchitecture-Level Perspective"

    // There are 2 entries waiting for wakeup per instruction window entry
    // The tag specifies a physical register and needs an extra bit to differentiate FP from integer registers
    // We can get a result from each functional unit every cycle
    // After XORing the tag bits, we need to combine to get a final match signal with 2-input gates
    // Thus we have x comparing gates and x - 1 combining gates per wakeup signal
    N_WINDOW_WAKEUP_DEVICES = cfg.WindowSize * 2 * ( // Number of entries that can be woken up
                                (cfg.NumIntAlu + cfg.NumIntMult + cfg.NumFpAdd + cfg.NumFpMult) * (2 * (1 + log2(cfg.NumIntPhysicalRegisters)) - 1) + // Checks if a single tag matches a single entry
                                (cfg.NumIntAlu + cfg.NumIntMult + cfg.NumFpAdd + cfg.NumFpMult) - 1 // Combines to check if any of the tags matches a single entry
                              ) * (K_STATIC * 2);

    // assume a similar model for the LSQ, 128 entries and 4 cache ports
    // Assume 160 bits per entry - address, data, and control
    N_LSQ_PREG_DEVICES = cfg.LsqSize * (64 + 64 + 32) * (K_D_LATCH * 10);
    //N_LSQ_SELECT_DEVICES = ((cfg.LsqSize - 1) / 7 * 111) * cfg.NumMemPort * (K_STATIC * 2); We don't even have LSQ select accesses so what was this?

    N_LSQ_WAKEUP_DEVICES = cfg.LsqSize * 2 * ( // Number of entries that can be woken up - data and address?
                             cfg.NumMemPort * (2 * (1 + log2(cfg.NumIntPhysicalRegisters)) - 1) + // Checks if a single tag matches a single entry
                             cfg.NumMemPort - 1 // Combines to check if any of the tags matches a single entry
                           ) * (K_STATIC * 2);

    // assume RAM for the reg file, 64 bits data
    N_INT_REGFILE_DEVICES = cfg.NumIntPhysicalRegisters * 64 * (K_RAM * 6);
    N_FP_REGFILE_DEVICES = cfg.NumFpPhysicalRegisters * 64 * (K_RAM * 6);

    N_ICACHE_DEVICES = cfg.L1iNumSets * cfg.L1iAssociativity * (cfg.L1iBlockSize + 2 + ceil(log2(factorial(cfg.L1iAssociativity)))) * (K_RAM * 6) + // Data, valid, dirty, LRU
                       cfg.L1iNumSets * cfg.L1iAssociativity * (64 - log2(cfg.L1iNumSets) - log2(cfg.L1iBlockSize)) * (K_RAM * 6); // Tag array

    N_DCACHE_DEVICES = cfg.L1dNumSets * cfg.L1dAssociativity * (cfg.L1dBlockSize + 2 + ceil(log2(factorial(cfg.L1dAssociativity)))) * (K_RAM * 6) + // Data, valid, dirty, LRU
                       cfg.L1dNumSets * cfg.L1dAssociativity * (64 - log2(cfg.L1dNumSets) - log2(cfg.L1dBlockSize)) * (K_RAM * 6); // Tag array

    N_UCACHE2_DEVICES = cfg.L2uNumSets * cfg.L2uAssociativity * (cfg.L2uBlockSize + 2 + ceil(log2(factorial(cfg.L2uAssociativity)))) * (K_RAM * 6) + // Data, valid, dirty, LRU
                        cfg.L2uNumSets * cfg.L2uAssociativity * (64 - log2(cfg.L2uNumSets) - log2(cfg.L2uBlockSize)) * (K_RAM * 6); // Tag array

    // This is still pretty arbitrary
    N_IALU_DEVICES = (double)10000 * (K_STATIC * 2);
    N_FALU_DEVICES = (double)20000 * (K_STATIC * 2);

    // In the Alpha 21264, a TLB entry contains ASN (8), prot (4), valid (1), tag (35) and physical addr (31)
    // Total size of a TLB entry is 79 bits
    // We'll use this for now - the TLBs are so small their leakage will be negligible anyway
    N_ITLB_DEVICES = cfg.ItlbNumSets * cfg.ItlbAssociativity * 79 * (K_RAM * 6);
    N_DTLB_DEVICES = cfg.DtlbNumSets * cfg.DtlbAssociativity * 79 * (K_RAM * 6);

    if (cfg.EnableTemperatureTracker) {
      hotSpotConfig.t_chip = cfg.HotSpotChipThickness;
      hotSpotConfig.c_convec = cfg.HotSpotConvectionCapacitance;
      hotSpotConfig.r_convec = cfg.HotSpotConvectionResistance;
      hotSpotConfig.s_sink = cfg.HotSpotHeatsinkSide;
      hotSpotConfig.t_sink = cfg.HotSpotHeatsinkThickness;
      hotSpotConfig.s_spreader = cfg.HotSpotHeatSpreaderSide;
      hotSpotConfig.t_spreader = cfg.HotSpotHeatSpreaderThickness;
      hotSpotConfig.t_interface = cfg.HotSpotInterfaceMaterialThickness;
      hotSpotConfig.ambient = cfg.HotSpotAmbientTemperature + 273.15;
      strcpy(hotSpotConfig.model_type, BLOCK_MODEL_STR);

      if (cfg.HotSpotInitialTemperatureFile != "") {
        strcpy(hotSpotConfig.init_file, cfg.HotSpotInitialTemperatureFile.c_str());
      } else {
        strcpy(hotSpotConfig.init_file, nullptrFILE);
      }
      if (cfg.HotSpotSteadyStateTemperatureFile != "") {
        strcpy(hotSpotConfig.steady_file, cfg.HotSpotSteadyStateTemperatureFile.c_str());
      } else {
        strcpy(hotSpotConfig.steady_file, nullptrFILE);
      }

      hotSpotConfig.init_temp = cfg.DefaultTemperature + 273.15; // Used if initial temperature file not specified
      hotSpotConfig.sampling_intvl = cfg.HotSpotInterval / cfg.Frequency;
      hotSpotConfig.base_proc_freq = cfg.Frequency;
      hotSpotConfig.dtm_used = false;
      hotSpotConfig.block_omit_lateral = false;

      temperatureTracker.initialize(hotSpotConfig, cfg.HotSpotFloorpanFile, cfg.HotSpotInterval, numCoreTiles, numL2Tiles);
    } else {
      temperatureTracker.setConstantTemperature(cfg.DefaultTemperature);
    }

    totalBaselineCycles = 0;
    updatePowerScalingFactors();

    resetAccessCounters();
    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nPowerTracker::PowerTrackerComponent::finalize, this ) );
  }

  void finalize() {
    if (cfg.EnableTemperatureTracker) {
      temperatureTracker.writeSteadyStateTemperatures();
    }
  }

private:
  int32_t pow2(int32_t y) {
    return static_cast<int>(pow(2.0, static_cast<double>(y)));
  }

  int32_t factorial(int32_t x) {
    int32_t y = 1;

    while (x > 1) {
      y *= x--;
    }

    return y;
  }

  void updatePower() {
    uint32_t i;

    for (i = 0; i < numCoreTiles; ++i) {
      powerUsed.addToL1iDynamic(i,   coreAccesses[i].l1iAccesses          * coreAccessCosts.getL1iPerAccessDynamic());
      powerUsed.addToItlbDynamic(i,   coreAccesses[i].l1iAccesses          * coreAccessCosts.getItlbPerAccessDynamic());
      powerUsed.addToBpredDynamic(i,   coreAccesses[i].bpredAccesses   * coreAccessCosts.getBpredPerAccessDynamic());
      powerUsed.addToIntRenameDynamic(i,  coreAccesses[i].intRenameAccesses  * coreAccessCosts.getIntRenamePerAccessDynamic());
      powerUsed.addToFpRenameDynamic(i,  coreAccesses[i].fpRenameAccesses  * coreAccessCosts.getFpRenamePerAccessDynamic());
      powerUsed.addToIntRegfileDynamic(i,  coreAccesses[i].intRegFileAccesses   * coreAccessCosts.getIntRegFilePerAccessDynamic());
      powerUsed.addToFpRegfileDynamic(i,  coreAccesses[i].fpRegFileAccesses  * coreAccessCosts.getFpRegFilePerAccessDynamic());
      powerUsed.addToIntAluDynamic(i,   coreAccesses[i].intAluAccesses   * coreAccessCosts.getIntAluPerAccessDynamic());
      powerUsed.addToFpAddDynamic(i,   coreAccesses[i].fpAddAccesses   * coreAccessCosts.getFpAddPerAccessDynamic());
      powerUsed.addToFpMulDynamic(i,   coreAccesses[i].fpMulAccesses   * coreAccessCosts.getFpMulPerAccessDynamic());
      powerUsed.addToL1dDynamic(i,   coreAccesses[i].l1dAccesses    * coreAccessCosts.getL1dPerAccessDynamic());
      powerUsed.addToDtlbDynamic(i,   coreAccesses[i].l1dAccesses    * coreAccessCosts.getDtlbPerAccessDynamic());
      powerUsed.addToLsqDynamic(i,   coreAccesses[i].lsqWakeupAccesses  * coreAccessCosts.getLsqWakeupPerAccessDynamic());
      powerUsed.addToLsqDynamic(i,   coreAccesses[i].lsqRsAccesses   * coreAccessCosts.getLsqRsPerAccessDynamic());
      powerUsed.addToResultBusDynamic(i,  coreAccesses[i].resultBusAccesses  * coreAccessCosts.getResultBusPerAccessDynamic());
      powerUsed.addToWindowDynamic(i,   (coreAccesses[i].intRsAccesses + coreAccesses[i].fpRsAccesses + coreAccesses[i].memRsAccesses)                      * coreAccessCosts.getRsPerAccessDynamic());
      powerUsed.addToWindowDynamic(i,   (coreAccesses[i].intSelectionAccesses + coreAccesses[i].fpSelectionAccesses + coreAccesses[i].memSelectionAccesses) * coreAccessCosts.getSelectionPerAccessDynamic());
      powerUsed.addToWindowDynamic(i,   (coreAccesses[i].intWakeupAccesses + coreAccesses[i].fpWakeupAccesses + coreAccesses[i].memWakeupAccesses)         * coreAccessCosts.getWakeupPerAccessDynamic());
      powerUsed.addToClockDynamic(i,   coreAccesses[i].clockTicks           * coreAccessCosts.getClockPerCycleDynamic());

      powerUsed.addToL1iLeakage(i, BASE_LEAKAGE_POWER_CACHE * N_ICACHE_DEVICES * coreLeakageScalingFactors[i].l1iFactor);
      powerUsed.addToItlbLeakage(i, BASE_LEAKAGE_POWER_CACHE * N_ITLB_DEVICES * coreLeakageScalingFactors[i].itlbFactor);
      powerUsed.addToBpredLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_BPRED_DEVICES * coreLeakageScalingFactors[i].bpredFactor);
      powerUsed.addToIntRenameLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_INT_RENAME_DEVICES * coreLeakageScalingFactors[i].intRenameFactor);
      powerUsed.addToFpRenameLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_FP_RENAME_DEVICES * coreLeakageScalingFactors[i].fpRenameFactor);
      powerUsed.addToIntRegfileLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_INT_REGFILE_DEVICES * coreLeakageScalingFactors[i].intRegfileFactor);
      powerUsed.addToFpRegfileLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_FP_REGFILE_DEVICES * coreLeakageScalingFactors[i].fpRegfileFactor);
      powerUsed.addToWindowLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_WINDOW_PREG_DEVICES * coreLeakageScalingFactors[i].windowFactor);
      powerUsed.addToWindowLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_WINDOW_SELECT_DEVICES * coreLeakageScalingFactors[i].windowFactor);
      powerUsed.addToWindowLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_WINDOW_WAKEUP_DEVICES * coreLeakageScalingFactors[i].windowFactor);
      powerUsed.addToIntAluLeakage(i, (cfg.NumIntAlu + cfg.NumIntMult) * BASE_LEAKAGE_POWER_LOGIC * N_IALU_DEVICES * coreLeakageScalingFactors[i].intAluFactor);
      powerUsed.addToFpAddLeakage(i, cfg.NumFpAdd * BASE_LEAKAGE_POWER_LOGIC * N_FALU_DEVICES * coreLeakageScalingFactors[i].fpAddFactor);
      powerUsed.addToFpMulLeakage(i, cfg.NumFpMult * BASE_LEAKAGE_POWER_LOGIC * N_FALU_DEVICES * coreLeakageScalingFactors[i].fpMulFactor);
      powerUsed.addToL1dLeakage(i, BASE_LEAKAGE_POWER_CACHE * N_DCACHE_DEVICES * coreLeakageScalingFactors[i].l1dFactor);
      powerUsed.addToDtlbLeakage(i, BASE_LEAKAGE_POWER_CACHE * N_DTLB_DEVICES * coreLeakageScalingFactors[i].dtlbFactor);
      powerUsed.addToLsqLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_LSQ_WAKEUP_DEVICES * coreLeakageScalingFactors[i].lsqFactor);
      powerUsed.addToLsqLeakage(i, BASE_LEAKAGE_POWER_LOGIC * N_LSQ_PREG_DEVICES * coreLeakageScalingFactors[i].lsqFactor);
      powerUsed.addToResultBusLeakage(i, TURNOFF_FACTOR * coreAccessCosts.getResultBusPerAccessDynamic() * coreLeakageScalingFactors[i].resultBusFactor);
    }
    for (i = 0; i < numL2Tiles; ++i) {
      powerUsed.addToL2uDynamic(i, l2Accesses[i] * l2AccessCost);
      powerUsed.addToL2uDynamic(i, l2ClockTicks[i] * l2ClockCost);

      powerUsed.addToL2uLeakage(i, BASE_LEAKAGE_POWER_CACHE * N_UCACHE2_DEVICES * l2LeakageScalingFactors[i]);
    }

    resetAccessCounters();
  }

  // This should line up with the stats interval or else things will go haywire
  void doInterval() {
    PerBlockPower intervalPower;
    uint32_t i;

    intervalPower = powerUsed - lastCheckpointPower;

    // Multiplying everything by 1000000 means we lose less accuracy when casting to int64_t, as int64_t as we remember to divide it back out at the end
    // This is what happens when you don't have a Stat::DoubleCounter
    dynamic_Stat += static_cast<int64_t>(1000000 * intervalPower.getTotalDynamic() / cfg.PowerStatInterval + 0.5);
    leakage_Stat += static_cast<int64_t>(1000000 * intervalPower.getTotalLeakage() / cfg.PowerStatInterval + 0.5);
    total_Stat += static_cast<int64_t>(1000000 * intervalPower.getTotalPower() / cfg.PowerStatInterval + 0.5);

    for (i = 0; i < numCoreTiles; ++i) {
      (*coreDynamic_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getCoreDynamic(i) / cfg.PowerStatInterval + 0.5);
      (*coreLeakage_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getCoreLeakage(i) / cfg.PowerStatInterval + 0.5);
      (*coreTotal_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getCorePower(i) / cfg.PowerStatInterval + 0.5);
    }
    for (i = 0; i < numL2Tiles; ++i) {
      (*l2Dynamic_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getL2uDynamic(i) / cfg.PowerStatInterval + 0.5);
      (*l2Leakage_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getL2uLeakage(i) / cfg.PowerStatInterval + 0.5);
      (*l2Total_Stats[i]) += static_cast<int64_t>(1000000 * intervalPower.getL2uPower(i) / cfg.PowerStatInterval + 0.5);
    }

    lastCheckpointPower = powerUsed;
    intervalCounter = 0;
  }

  void doHotSpot() {
    PerBlockPower intervalPower;
    intervalPower = (powerUsed - lastHotSpotPower);

    temperatureTracker.updateTemperature(intervalPower);

    lastHotSpotPower = powerUsed;
    hotSpotCounter = 0;
  }

  void updatePowerScalingFactors() {
    std::string index;
    std::ostringstream ossIndex;
    uint32_t i;

    for (i = 0; i < numCoreTiles; ++i) {
      ossIndex.str("");
      ossIndex << i;
      index = ossIndex.str();

      coreLeakageScalingFactors[i].l1iFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-Icache"));
      coreLeakageScalingFactors[i].itlbFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-ITB"));
      coreLeakageScalingFactors[i].bpredFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-Bpred"));
      coreLeakageScalingFactors[i].intRenameFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-IntMap"));
      coreLeakageScalingFactors[i].fpRenameFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-FPMap"));
      coreLeakageScalingFactors[i].intRegfileFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-IntReg"));
      coreLeakageScalingFactors[i].fpRegfileFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-FPReg"));
      coreLeakageScalingFactors[i].windowFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-Window"));
      coreLeakageScalingFactors[i].intAluFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-IntExec"));
      coreLeakageScalingFactors[i].fpAddFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-FPAdd"));
      coreLeakageScalingFactors[i].fpMulFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-FPMul"));
      coreLeakageScalingFactors[i].l1dFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-Dcache"));
      coreLeakageScalingFactors[i].dtlbFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-DTB"));
      coreLeakageScalingFactors[i].lsqFactor = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-LdStQ"));
      coreLeakageScalingFactors[i].resultBusFactor = (coreLeakageScalingFactors[i].intRegfileFactor + coreLeakageScalingFactors[i].fpRegfileFactor) / 2;
    }
    for (i = 0; i < numL2Tiles; ++i) {
      ossIndex.str("");
      ossIndex << i;
      index = ossIndex.str();

      l2LeakageScalingFactors[i] = getLeakagePowerScalingFactor(temperatureTracker.getKelvinTemperature(index + "-L2"));
    }
  }

  void resetAccessCounters() {
    std::fill(coreAccesses.begin(), coreAccesses.end(), CoreAccessCounts());
    std::fill(l2Accesses.begin(), l2Accesses.end(), 0);
    std::fill(l2ClockTicks.begin(), l2ClockTicks.end(), 0);
  }

  // FIXME: Insert your favorite function for scaling leakage power for temperature. Baseline temperature in Kelvin is BASE_LEAKAGE_TEMP
  double getLeakagePowerScalingFactor(const double temperature) {
    return 1;
  }

public:
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(DispatchedInstructionIn);
  void push( interface::DispatchedInstructionIn const &, index_t coreIndex, int64_t & opcode) {
    if (cfg.EnablePowerTracker) {
      incrementActivityCounters(coreIndex, opcode);
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L1dRequestIn);
  void push( interface::L1dRequestIn const &, index_t coreIndex, bool & garbage ) {
    ++coreAccesses[coreIndex].l1dAccesses;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2uRequestIn);
  void push( interface::L2uRequestIn const &, index_t l2Index, bool & garbage ) {
    ++l2Accesses[l2Index];
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(StoreForwardingHitIn);
  void push( interface::StoreForwardingHitIn const &, index_t coreIndex, bool & garbage ) {
    ++coreAccesses[coreIndex].lsqRsAccesses;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L1iRequestIn);
  void push( interface::L1iRequestIn const &, index_t coreIndex, bool & garbage ) {
    ++coreAccesses[coreIndex].l1iAccesses;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(CoreClockTickIn);
  void push( interface::CoreClockTickIn const &, index_t coreIndex, bool & garbage ) {
    ++coreAccesses[coreIndex].clockTicks;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2ClockTickIn);
  void push( interface::L2ClockTickIn const &, index_t l2Index, bool & garbage ) {
    ++l2ClockTicks[l2Index];
  }

public:
  void drive(interface::PowerTrackerDrive const &) {
    ++totalBaselineCycles;

    if (cfg.EnablePowerTracker) {
      temperatureTracker.addToCycles(1);
      updatePower();

      if (++hotSpotCounter == cfg.HotSpotInterval && cfg.EnableTemperatureTracker) {
        doHotSpot();
        updatePowerScalingFactors();
      }

      // Write power stats for each interval
      if (++intervalCounter == cfg.PowerStatInterval) {
        doInterval();
      }
    }

    if (totalBaselineCycles == cfg.HotSpotResetCycle && cfg.EnableTemperatureTracker) {
      temperatureTracker.reset();
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(CoreAveragePowerOut);
  double pull( interface::CoreAveragePowerOut const &, index_t coreIndex ) {
    double corePower;
    double toReturn;

    corePower = powerUsed.getCorePower(coreIndex);

    toReturn = (corePower - lastCoreRequestPowers[coreIndex]) / (totalBaselineCycles - lastCoreRequestCycles[coreIndex]);
    lastCoreRequestPowers[coreIndex] = corePower;
    lastCoreRequestCycles[coreIndex] = totalBaselineCycles;

    return toReturn;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2AveragePowerOut);
  double pull( interface::L2AveragePowerOut const &, index_t l2Index ) {
    double l2Power;
    double toReturn;

    l2Power = powerUsed.getL2uPower(l2Index);

    toReturn = (l2Power - lastL2RequestPowers[l2Index]) / (totalBaselineCycles - lastL2RequestCycles[l2Index]);
    lastL2RequestPowers[l2Index] = l2Power;
    lastL2RequestCycles[l2Index] = totalBaselineCycles;

    return toReturn;
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(CoreTemperatureOut);
  double pull( interface::CoreTemperatureOut const &, index_t coreIndex ) {
    return temperatureTracker.getCoreTemperature(coreIndex);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2TemperatureOut);
  double pull( interface::L2TemperatureOut const &, index_t l2Index) {
    return temperatureTracker.getL2Temperature(l2Index);
  }

private:
  // Takes a SPARC V9 instruction word and increments appropriate activity counters
  void incrementActivityCounters(uint32_t coreIndex, int64_t opcode) {
    // Instruction is 32 bits of SPARC v9 goodness
    // Note that we track some things separately (such as int/fp wakeup accesses) even though
    // they are lumped into one power number (i.e., they increment the same power counter by the
    // same amount). This makes it easier to extend the model to a clustered architecture in
    // the future for relatively little effort now.

    // Various possible fields used to identify instructions
    // Warning, some have the same names in the manual but represent DIFFERENT bits.
    uint32_t op = (opcode >> 30) & 0x3;      // [31:30]
    uint32_t fcn = (opcode >> 25) & 0x1f;    // [29:25], used for DONE and RETRY
    //uint32_t cond = (opcode >> 25) & 0xf;    // [28:25]
    uint32_t rcond = (opcode >> 25) & 0x7;   // [27:25] for branches. WARNING: used for other instructions
    //         but may NOT be 27:25 (A.34, A.36 are examples)
    uint32_t op2 = (opcode >> 22) & 0x7;     // [24:22]
    uint32_t op3 = (opcode >> 19) & 0x3f;    // [24:19]
    uint32_t iField = (opcode >> 13) & 0x1;  // [13], this one is called just 'i' in the manual
    //uint32_t xField = (opcode >> 12) & 0x1;
    uint32_t opf = (opcode >> 5) & 0x1ff;    // [13:5], for fp

    uint32_t rs1 = (opcode >> 14) & 0x1f;    // [18:14]
    uint32_t rs2 = opcode & 0x1f;            // [ 4: 0]
    uint32_t rd = fcn;

    // - The current format is the following:  instructions in this decoder appear in the
    //   same order they appear in the manual.  All instructions are grouped by section
    //   starting at the comment indicating the section number (eg A.2) and the section
    //   name in the manual (eg "Add").
    //
    // - As far as I can tell every instruction within the same section increments the same
    //   counters.
    //
    // - This is currently one very big if-else block. We will assume that the compiler will
    //   optimize away some of the inefficiencies (for example, many operation types can be
    //   combined based on a single field or two).

    // A.2 - Add
    if ((op == 2) && ((op3 == 0) || (op3 == 0x10) || (op3 == 0x8) || (op3 == 0x18)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.3 - Branch on Integer Register with Prediction
    else if ((op == 0) && (op2 == 0x3) && (((opcode >> 28) & 1) == 0)) {
      if ((rcond != 0) && (rcond != 0x4)) { // 0 and 4 are rsvd opcodes
        ++coreAccesses[coreIndex].bpredAccesses;
        if (rs1 != 0) {
          ++coreAccesses[coreIndex].intRegFileAccesses;
          ++coreAccesses[coreIndex].intRsAccesses;
        }
        ++coreAccesses[coreIndex].intAluAccesses;
      }
    }
    // A.4 - Branch on FP Condition Codes
    else if ((op == 0) && (op2 == 0x6)) {
      ++coreAccesses[coreIndex].bpredAccesses;
    }
    // A.5 - Branch on FP Condition Codes with Prediction
    else if ((op == 0) && (op2 == 0x5)) {
      ++coreAccesses[coreIndex].bpredAccesses;
    }
    // A.6 - Branch on Integer Condition Codes
    else if ((op == 0) && (op2 == 0x2)) {
      ++coreAccesses[coreIndex].bpredAccesses;
    }
    // A.7 - Branch on Integer Condition Codes with Prediction
    else if ((op == 0) && (op2 == 0x1)) {
      ++coreAccesses[coreIndex].bpredAccesses;
    }
    // A.8 - Call and Link
    else if (op == 0x1) {
      ++coreAccesses[coreIndex].bpredAccesses;
    }
    // A.9 - Compare and Swap
    else if ((op == 0x3) && ((op3 == 0x3c) || (op3 == 0x3e)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load

      // We can't know if the store happens or not - assume not?
      //++coreAccesses[coreIndex].lsqRsAccesses; //Once per store

      // always 2 reg operand reads
      if (rs1 != 0) {
        ++coreAccesses[coreIndex].intRegFileAccesses;
        ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
      }
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].intRegFileAccesses;
        ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
      }
    }
    // A.10 Divide
    else if ((op == 0x2) && ((op3 == 0xe) || (op3 == 0xf) || (op3 == 0x1e) || (op3 == 0x1f)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.11 - DONE and RETRY
    //else if ((op == 0x2) && (op3 == 0x3e)) {
    //}
    // A.12 - Floating-Point Add and Subtract
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x41) || (opf == 0x42) || (opf == 0x43) || (opf == 0x45) || (opf == 0x46) || (opf == 0x47))) {
      fpIncrementForWrite(coreIndex, rd);
      fpIncrementForRead(coreIndex, (uint32_t)0, rs1, rs2);
      ++coreAccesses[coreIndex].fpAddAccesses;

    }
    // A.13 - Floating-Point Compare
    else if ((op == 0x2) && (((opcode >> 27) & 0x7) == 0) && (op3 == 0x35) && ((opf == 0x51) || (opf == 0x52) || (opf == 0x53) || (opf == 0x55) || (opf == 0x56) || (opf == 0x57))) {
      fpIncrementForRead(coreIndex, (uint32_t)0, rs1, rs2);
      ++coreAccesses[coreIndex].fpAddAccesses;
    }
    // A.14 - Convert Floating-Point to Integer
    // Read from FP reg, write into FP reg
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x81) || (opf == 0x82) || (opf == 0x83) || (opf == 0xd1) || (opf == 0xd2) || (opf == 0xd3))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
      ++coreAccesses[coreIndex].fpAddAccesses;
    }
    // A.15 - Convert Between Floating-Point Formats
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0xc9) || (opf == 0xcd) || (opf == 0xc6) || (opf == 0xce) || (opf == 0xc7) || (opf == 0xcb))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
      ++coreAccesses[coreIndex].fpAddAccesses;
    }
    // A.16 - Convert Integer To Floating-Point
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x84) || (opf == 0x88) || (opf == 0x8c) || (opf == 0xc4) || (opf == 0xc8) || (opf == 0xcc))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
      ++coreAccesses[coreIndex].fpAddAccesses;
    }
    // A.17 - Floating-Point Move
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x1) || (opf == 0x2) || (opf == 0x3) || (opf == 0x5)  || (opf == 0x6) || (opf == 0x7) || (opf == 0x9) || (opf == 0xa) || (opf == 0xb))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
      // Assume that we would have logic to handle complementing or clearing sign bit
      // (NEG and ABS) versions without using a whole FP ALU
    }
    // A.18 - Floating-Point Multiply and Divide
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x49) || (opf == 0x4a)  || (opf == 0x4b) || (opf == 0x69) || (opf == 0x6e) || (opf == 0x4d)  || (opf == 0x4e)  || (opf == 0x4f))) {
      fpIncrementForWrite(coreIndex, rd);
      fpIncrementForRead(coreIndex, (uint32_t)0, rs1, rs2);
      ++coreAccesses[coreIndex].fpMulAccesses;
    }
    // A.19 - Floating-Point Square Root
    else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x29) || (opf == 0x2a) || (opf == 0x2b))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
      ++coreAccesses[coreIndex].fpMulAccesses;
    }
    // A.20 - Flush Instruction Memory
    else if ((op == 0x2) && (op3 == 0x3b)) {
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.21 - Flush Register Windows
    //else if ((op == 0x2) && (op3 == 0x2b) && (((opcode >> 13) & 1) == 0) ) {
    //}
    // A.22 - Illegal Instruction Trap
    //else if ((op == 0) && (op2 == 0)) {
    //}
    // A.23 - Implementation-Dependent Instructions
    //else if ((op == 0x2) && (op3 == 0x36)) {
    //}
    //else if ((op == 0x2) && (op3 == 0x37)) {
    //}
    // A.24 - Jump and Link
    else if ((op == 0x2) && (op3 == 0x38)) {
      ++coreAccesses[coreIndex].bpredAccesses;

      // PC copied and written into regfile
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.25 - Load Floating-Point
    // Note: field called "rd" for this instr, but occupies same bits as other instr's "fcn"
    else if ((op == 0x3) && ((op3 == 0x20) || (op3 == 0x23) || (op3 == 0x22) || (op3 == 0x21))) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      fpIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.26 - Load Floating-Point from Alternate Space
    else if ((op == 0x3) && ((op3 == 0x30) || (op3 == 0x33) || (op3 == 0x32)) ) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      fpIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.27 Load Integer
    else if ((op == 0x3) && ((op3 == 0x9) || (op3 == 0xa) || (op3 == 0x8) || (op3 == 0x1) || (op3 == 0x2) || (op3 == 0x0) || (op3 == 0xb) || (op3 == 0x3))) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.28 - Load Integer from Alternate Space
    else if ((op == 0x3) && ((op3 == 0x19) || (op3 == 0x1a) || (op3 == 0x18) || (op3 == 0x11) || (op3 == 0x12) || (op3 == 0x10) || (op3 == 0x1b) || (op3 == 0x13))) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.29 - Load-Store Unsigned Byte
    else if ((op == 0x3) && (op3 == 0xd)) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.30 - Load-Store Unsigned Byte to Alternate Space
    else if ((op == 0x3) && (op3 == 0x1d)) {
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.31 - Logical Operations
    else if ((op == 0x2) && ((op3 == 0x1) || (op3 == 0x11)  || (op3 == 0x5)  || (op3 == 0x15) || (op3 == 0x2)  || (op3 == 0x12)  || (op3 == 0x6)  || (op3 == 0x16) || (op3 == 0x3)  || (op3 == 0x13)  || (op3 == 0x7)  || (op3 == 0x17))) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.32 - Memory Barrier
    //else if ((op == 0x2) && (op3 == 0x28) && (fcn == 0) && (((opcode >> 14) & 0x1f) ==0xf) && ((opcode >> 13) & 1)) {
    //}
    // A.33 - Move Floating-Point Register on Condition
    else if ( (op == 0x2) && (((opcode >> 18) & 1) == 0) && (op3 == 0x35) && ((((opcode >> 5) & 0x3f) == 1) || (((opcode >> 5) & 0x3f) == 2) || (((opcode >> 5) & 0x3f) == 3))) {
      fpIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].fpRegFileAccesses;
        ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
      }
    }
    // A.34 - Move F-P Register on Integer Register Condtion (FMOVr)
    else if ((op == 0x2) && (op3 == 0x35) && (iField == 0) && ((((opcode >> 5) & 0x1f) == 5) || (((opcode >> 5) & 0x1f) == 6) || (((opcode >> 5) & 0x1f) == 7))) {
      // This field is called "rcond" in this section, however "rcond" is used for different bits for branch instructions
      uint32_t a34 = (opcode >> 10) & 7;
      if ((a34 != 0) && (a34 != 4)) {
        fpIncrementForWrite(coreIndex, rd);
        if (rs1 != 0) {
          ++coreAccesses[coreIndex].intRegFileAccesses;
          ++coreAccesses[coreIndex].intRsAccesses;
        }
        if (rs2 != 0) {
          ++coreAccesses[coreIndex].fpRegFileAccesses;
          ++coreAccesses[coreIndex].fpRsAccesses;
        }
        ++coreAccesses[coreIndex].intAluAccesses;
      }
    }
    // A.35 - Move Integer Register on Condition
    else if ((op == 0x2) && (op3 == 0x2c)) {
      intIncrementForWrite(coreIndex, rd);
      if (rs2 != 0) {
        ++coreAccesses[coreIndex].intRegFileAccesses;
        ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
      }
    }
    // A.36 - Move Integer Register on Register Condition
    else if ((op == 0x2) && (op3 == 0x2f)) {
      // This field is called "rcond" in this section, however "rcond" is used for different bits for branch instructions
      uint32_t a36 = (opcode >> 10) & 7;
      if ((a36 != 0) && (a36 != 4)) {  // 0 and 4 are reserved
        intIncrementForWrite(coreIndex, rd);
        ++coreAccesses[coreIndex].intAluAccesses;
        intIncrementForRead(coreIndex, iField, rs1, rs2);
      }
    }
    // A.37 - Multiply and Divide (64)
    else if ((op == 0x2) && ((op3 == 0x9) || (op3 == 0x2d) || (op3 == 0xd)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.38 - Multiply (32)
    else if ((op == 0x2) && ((op3 == 0xa) || (op3 == 0xb) || (op3 == 0x1a) || (op3 == 0x1b)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.39 - Multiply Step
    else if ((op == 0x2) && (op3 == 0x24)) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.40 - No Operation
    //else if (opcode == 0x01000000) {
    //}
    // A.41 - Population Count
    else if ((op == 0x2) && (op3 == 0x2e) && (((opcode >> 14) & 0x1f) == 0) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      if (iField == 0 && rs2 != 0) {
        ++coreAccesses[coreIndex].intRegFileAccesses;
        ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
      }
    }
    // A.42 - Prefetch Data
    else if ((op == 0x3) && ((op3 == 0x2d) || (op3 == 0x3d)) ) {
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.43 - Read Privileged Register
    else if ((op == 0x2) && (op3 == 0x2a)) {
      intIncrementForWrite(coreIndex, rd);
    }
    // A.44 - Read State Register
    else if ((op == 0x2) && (((opcode >> 13) & 1) == 0) && (op3 == 0x28) && (((opcode >> 14) & 0x1f) != 15) ) {
      // We don't track reads of the state registers themselves
      intIncrementForWrite(coreIndex, rd);
    }
    // A.45 - Return
    else if ((op == 0x2) && (op3 == 0x39)) {
      ++coreAccesses[coreIndex].bpredAccesses;
      ++coreAccesses[coreIndex].intAluAccesses; // Compute target address
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.46 - Save and Restore
    else if ((op == 0x2) && (op3 == 0x3c)) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    } else if ((op == 0x2) && (op3 == 0x3d)) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.47 - Saved and Restored
    //else if ((op == 0x2) && (op3 == 0x31)) {
    //}
    // A.48 - SETHI
    else if ((op == 0) && (op2 == 0x4)) {
      // NOP is a special case of SETHI
      // If this condition is not true, the instruction is a NOP and we assume it has been optimized to actually do nothing
      if ((fcn != 0) && ((opcode && 0x1fffff) == 0)) {
        intIncrementForWrite(coreIndex, rd);
        ++coreAccesses[coreIndex].intAluAccesses;
        // Reads the same register it writes
        if (rd != 0) {
          ++coreAccesses[coreIndex].intRegFileAccesses;
          ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
        }
      }
    }
    // A.49 - Shift
    else if ((op == 0x2) && ((op3 == 0x25) || (op3 == 0x26) || (op3 == 0x27)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.50 - Software-Initiated Reset
    //else if ((op == 0x2) && (fcn == 0xf) && (((opcode >> 14) & 0x1f) == 0)) {
    //}
    // A.51 - Store Barrier
    //else if ((op == 0x2) && (fcn == 0x0) && (((opcode >> 13) & 0x1f) == 0x1e) && (op3 == 0x28)) {
    //}
    // A.52 - Store Floating-Point
    else if ((op == 0x3) && ((op3 == 0x24) || (op3 == 0x27) || (op3 == 0x26) || (op3 == 0x25)) ) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.53 - Store Floating-Point into Alternate Space
    else if ((op == 0x3) && ((op3 == 0x34) || (op3 == 0x37) || (op3 == 0x36)) ) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.54 - Store Integer
    else if ((op == 0x3) && ((op3 == 0x5) || (op3 == 0x6) || (op3 == 0x4) || (op3 == 0xe) || (op3 == 0x7)) ) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.55 - Store Integer into Alternate Space
    else if ((op == 0x3) && ((op3 == 0x15) || (op3 == 0x16) || (op3 == 0x14) || (op3 == 0x1e) || (op3 == 0x17)) ) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.56 - Subtract
    else if ((op == 0x2) && ( (op3 == 0x4) || (op3 == 0x14) || (op3 == 0xc) || (op3 == 0x1c)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.57 - Swap Register with Memory
    else if ((op == 0x3) && (op3 == 0xf)) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.58 - Swap Register with Alternate Space Memory
    else if ((op == 0x3) && (op3 == 0x1f)) {
      ++coreAccesses[coreIndex].lsqRsAccesses; //Once per store
      ++coreAccesses[coreIndex].lsqWakeupAccesses; //Once per load
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.59 - Tagged Add
    else if ((op == 0x2) && ( (op3 == 0x20) || (op3 == 0x22)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.60 - Tagged Subtract
    else if ((op == 0x2) && ((op3 == 0x21) || (op3 == 0x23)) ) {
      intIncrementForWrite(coreIndex, rd);
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.61 - Trap on Integer Condition Codes
    else if ((op == 0x2) && (op3 == 0x3a)) {
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.62 - Write Privileged Register
    else if ((op == 0x2) && (op3 == 0x32)) {
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
    // A.63 - Write State Register
    else if ((op == 0x2) && (op3 == 0x30)) {
      ++coreAccesses[coreIndex].intAluAccesses;
      intIncrementForRead(coreIndex, iField, rs1, rs2);
    }
  }

  /************************* Helper Functions ***************************/

  /* This function is only for integer operands.
   * This function increments the appropriate number of counters for instructions that
   * may read 2 register operands or 1 register operand and one immediate operand.  This
   * is only called for those types on instructions.
   *
   * Arguments:
   *   iField: bit 13 in opcode, when 0 in these instructions there are 2 reg operands
   *       rd: [29:25]
   *      rs1: [18:14]
   *      rs2: [ 4: 0]
   */
  void intIncrementForRead(uint32_t coreIndex, uint32_t iField, uint32_t rs1, uint32_t rs2) {
    // One or two register operands
    if (rs1 != 0) {
      ++coreAccesses[coreIndex].intRegFileAccesses;
      ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
    }

    // Two register operands
    if (iField == 0 && rs2 != 0) {
      ++coreAccesses[coreIndex].intRegFileAccesses;
      ++coreAccesses[coreIndex].intRsAccesses; // One access on dispatch
    }
  }

  /* This is only for integer register destinations.
   * Increments appropriate counters for regfile write.  Does not increment
   * if destination register is r0.
   *
   */
  void intIncrementForWrite(uint32_t coreIndex, uint32_t rd) {
    if (rd != 0) {
      ++coreAccesses[coreIndex].intRenameAccesses;
      ++coreAccesses[coreIndex].intRegFileAccesses;
      ++coreAccesses[coreIndex].intSelectionAccesses; // One access on issue
      ++coreAccesses[coreIndex].intWakeupAccesses; // One access on writeback
      ++coreAccesses[coreIndex].intRsAccesses; // One access on writeback
      coreAccesses[coreIndex].intRsAccesses += 2; // Two accesses on issue
      ++coreAccesses[coreIndex].resultBusAccesses;
    }
  }

  /* Basically the same as the integer version but for fp.
   * Some fp instrutions always have 2 reg operands.  For these this
   * function is called with iField set to 0.
   */
  void fpIncrementForRead(uint32_t coreIndex, uint32_t iField, uint32_t rs1, uint32_t rs2) {
    if (rs1 != 0) {
      ++coreAccesses[coreIndex].fpRegFileAccesses;
      ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
    }
    if (iField == 0 && rs2 != 0) {
      ++coreAccesses[coreIndex].fpRegFileAccesses;
      ++coreAccesses[coreIndex].fpRsAccesses; // One access on dispatch
    }
  }

  /* Basically same as integer version but for fp.
   *
   */
  void fpIncrementForWrite(uint32_t coreIndex, uint32_t rd) {
    if (rd != 0) {
      ++coreAccesses[coreIndex].fpRenameAccesses;
      ++coreAccesses[coreIndex].fpRegFileAccesses;
      ++coreAccesses[coreIndex].fpSelectionAccesses; // One access on issue
      ++coreAccesses[coreIndex].fpWakeupAccesses; // One access on writeback
      ++coreAccesses[coreIndex].fpRsAccesses; // One access on writeback
      coreAccesses[coreIndex].fpRsAccesses += 2; // Two accesses on issue
      ++coreAccesses[coreIndex].resultBusAccesses;
    }
  }
};
}//End namespace nPowerTracker

FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L1iRequestIn) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, DispatchedInstructionIn) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L1dRequestIn) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, StoreForwardingHitIn) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, CoreClockTickIn) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L2uRequestIn) {
  return cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L2ClockTickIn) {
  return cfg.NumL2Tiles;
}

FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, CoreAveragePowerOut) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L2AveragePowerOut) {
  return cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, CoreTemperatureOut) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( PowerTracker, L2TemperatureOut) {
  return cfg.NumL2Tiles;
}

FLEXUS_COMPONENT_INSTANTIATOR(PowerTracker, nPowerTracker);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PowerTracker

#define DBG_Reset
#include DBG_Control()
