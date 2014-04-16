#ifndef FLEXUS_PER_BLOCK_POWER_HPP_INCLUDED
#define FLEXUS_PER_BLOCK_POWER_HPP_INCLUDED

class PerBlockPower {
  struct CoreBlockPowers {
    double  l1iDynamic,
         itlbDynamic,
         bpredDynamic,
         intRenameDynamic,
         fpRenameDynamic,
         intRegfileDynamic,
         fpRegfileDynamic,
         windowDynamic,
         intAluDynamic,
         fpAddDynamic,
         fpMulDynamic,
         l1dDynamic,
         dtlbDynamic,
         lsqDynamic,
         resultBusDynamic,
         clockDynamic,
         l1iLeakage,
         itlbLeakage,
         bpredLeakage,
         intRenameLeakage,
         fpRenameLeakage,
         intRegfileLeakage,
         fpRegfileLeakage,
         windowLeakage,
         intAluLeakage,
         fpAddLeakage,
         fpMulLeakage,
         l1dLeakage,
         dtlbLeakage,
         lsqLeakage,
         resultBusLeakage,
         clockLeakage;
  };

public:
  PerBlockPower();
  ~PerBlockPower();
  void setNumTiles(const uint32_t newNmCoreTiles, const uint32_t newNumL2Tiles);

  // Get individual block dynamic or static power value
  inline double getL1iDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1iDynamic;
  };
  inline double getItlbDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].itlbDynamic;
  };
  inline double getBpredDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].bpredDynamic;
  };
  inline double getIntRenameDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRenameDynamic;
  };
  inline double getFpRenameDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRenameDynamic;
  };
  inline double getIntRegfileDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRegfileDynamic;
  };
  inline double getFpRegfileDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRegfileDynamic;
  };
  inline double getWindowDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].windowDynamic;
  };
  inline double getIntAluDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intAluDynamic;
  };
  inline double getFpAddDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpAddDynamic;
  };
  inline double getFpMulDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpMulDynamic;
  };
  inline double getL1dDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1dDynamic;
  };
  inline double getDtlbDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].dtlbDynamic;
  };
  inline double getLsqDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].lsqDynamic;
  };
  inline double getResultBusDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].resultBusDynamic;
  }
  inline double getClockDynamic(const uint32_t coreIndex) const {
    return corePowers[coreIndex].clockDynamic;
  };
  inline double getL1iLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1iLeakage;
  };
  inline double getItlbLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].itlbLeakage;
  };
  inline double getBpredLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].bpredLeakage;
  };
  inline double getIntRenameLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRenameLeakage;
  };
  inline double getFpRenameLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRenameLeakage;
  };
  inline double getIntRegfileLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRegfileLeakage;
  };
  inline double getFpRegfileLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRegfileLeakage;
  };
  inline double getWindowLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].windowLeakage;
  };
  inline double getIntAluLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intAluLeakage;
  };
  inline double getFpAddLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpAddLeakage;
  };
  inline double getFpMulLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpMulLeakage;
  };
  inline double getL1dLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1dLeakage;
  };
  inline double getDtlbLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].dtlbDynamic;
  };
  inline double getLsqLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].lsqLeakage;
  };
  inline double getResultBusLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].resultBusLeakage;
  }
  inline double getClockLeakage(const uint32_t coreIndex) const {
    return corePowers[coreIndex].clockLeakage;
  }

  // Get individual block total dynamic plus static power
  inline double getL1iPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1iDynamic + corePowers[coreIndex].l1iLeakage;
  };
  inline double getItlbPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].itlbDynamic + corePowers[coreIndex].itlbLeakage;
  };
  inline double getBpredPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].bpredDynamic + corePowers[coreIndex].bpredLeakage;
  };
  inline double getIntRenamePower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRenameDynamic + corePowers[coreIndex].intRenameLeakage;
  };
  inline double getFpRenamePower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRenameDynamic + corePowers[coreIndex].fpRenameLeakage;
  };
  inline double getIntRegfilePower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intRegfileDynamic + corePowers[coreIndex].intRegfileLeakage;
  };
  inline double getFpRegfilePower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpRegfileDynamic + corePowers[coreIndex].fpRegfileLeakage;
  };
  inline double getWindowPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].windowDynamic + corePowers[coreIndex].windowLeakage;
  };
  inline double getIntAluPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].intAluDynamic + corePowers[coreIndex].intAluLeakage;
  };
  inline double getFpAddPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpAddDynamic + corePowers[coreIndex].fpAddLeakage;
  };
  inline double getFpMulPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].fpMulDynamic + corePowers[coreIndex].fpMulLeakage;
  };
  inline double getL1dPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].l1dDynamic + corePowers[coreIndex].l1dLeakage;
  };
  inline double getDtlbPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].dtlbDynamic + corePowers[coreIndex].dtlbLeakage;
  };
  inline double getLsqPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].lsqDynamic + corePowers[coreIndex].lsqLeakage;
  };
  inline double getResultBusPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].resultBusDynamic + corePowers[coreIndex].resultBusLeakage;
  };
  inline double getClockPower(const uint32_t coreIndex) const {
    return corePowers[coreIndex].clockDynamic + corePowers[coreIndex].clockLeakage;
  };

  // Add to power values
  inline void addToL1iDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].l1iDynamic += valueToAdd;
  };
  inline void addToItlbDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].itlbDynamic += valueToAdd;
  };
  inline void addToBpredDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].bpredDynamic += valueToAdd;
  };
  inline void addToIntRenameDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intRenameDynamic += valueToAdd;
  };
  inline void addToFpRenameDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpRenameDynamic += valueToAdd;
  };
  inline void addToIntRegfileDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intRegfileDynamic += valueToAdd;
  };
  inline void addToFpRegfileDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpRegfileDynamic += valueToAdd;
  };
  inline void addToWindowDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].windowDynamic += valueToAdd;
  };
  inline void addToIntAluDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intAluDynamic += valueToAdd;
  };
  inline void addToFpAddDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpAddDynamic += valueToAdd;
  };
  inline void addToFpMulDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpMulDynamic += valueToAdd;
  };
  inline void addToL1dDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].l1dDynamic += valueToAdd;
  };
  inline void addToDtlbDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].dtlbDynamic += valueToAdd;
  };
  inline void addToClockDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].clockDynamic += valueToAdd;
  };
  inline void addToLsqDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].lsqDynamic += valueToAdd;
  };
  inline void addToResultBusDynamic(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].resultBusDynamic += valueToAdd;
  };
  inline void addToL1iLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].l1iLeakage += valueToAdd;
  };
  inline void addToItlbLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].itlbLeakage += valueToAdd;
  };
  inline void addToBpredLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].bpredLeakage += valueToAdd;
  };
  inline void addToIntRenameLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intRenameLeakage += valueToAdd;
  };
  inline void addToFpRenameLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpRenameLeakage += valueToAdd;
  };
  inline void addToIntRegfileLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intRegfileLeakage += valueToAdd;
  };
  inline void addToFpRegfileLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpRegfileLeakage += valueToAdd;
  };
  inline void addToWindowLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].windowLeakage += valueToAdd;
  };
  inline void addToIntAluLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].intAluLeakage += valueToAdd;
  };
  inline void addToFpAddLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpAddLeakage += valueToAdd;
  };
  inline void addToFpMulLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].fpMulLeakage += valueToAdd;
  };
  inline void addToL1dLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].l1dLeakage += valueToAdd;
  };
  inline void addToDtlbLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].dtlbDynamic += valueToAdd;
  };
  inline void addToLsqLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].lsqLeakage += valueToAdd;
  };
  inline void addToResultBusLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].resultBusLeakage += valueToAdd;
  };
  inline void addToClockLeakage(const uint32_t coreIndex, const double valueToAdd) {
    corePowers[coreIndex].clockLeakage += valueToAdd;
  };
  inline void addToL2uDynamic(const uint32_t l2Index, const double valueToAdd) {
    l2uDynamic[l2Index] += valueToAdd;
  };
  inline void addToL2uLeakage(const uint32_t l2Index, const double valueToAdd) {
    l2uLeakage[l2Index] += valueToAdd;
  };

  // Get power for a single core tile
  double getCoreDynamic(const uint32_t i) const;
  double getCoreLeakage(const uint32_t i) const;
  double getCorePower(const uint32_t i) const;

  // Get power for a single L2 tile
  inline double getL2uDynamic(const uint32_t l2Index) const {
    return l2uDynamic[l2Index];
  };
  inline double getL2uLeakage(const uint32_t l2Index) const {
    return l2uLeakage[l2Index];
  };
  inline double getL2uPower(const uint32_t l2Index) const {
    return l2uDynamic[l2Index] + l2uLeakage[l2Index];
  };

  double getTotalDynamic() const;
  double getTotalLeakage() const;
  double getTotalPower() const;

  PerBlockPower & operator=(const PerBlockPower & source);
  PerBlockPower operator-(const PerBlockPower & p2) const;

private:
  uint32_t numCoreTiles;
  uint32_t numL2Tiles;

  CoreBlockPowers * corePowers;
  double * l2uDynamic,
         *l2uLeakage;
};

#endif
