#include <cassert>
#include <cstring>
#include "PerBlockPower.hpp"

PerBlockPower::PerBlockPower() :
  numCoreTiles(0),
  numL2Tiles(0),
  corePowers(0), // NULL
  l2uDynamic(0), // NULL
  l2uLeakage(0)  // NULL
{ }

PerBlockPower::~PerBlockPower() {
  if (corePowers) {
    delete[] corePowers;
  }
  if (l2uDynamic) {
    delete[] l2uDynamic;
  }
  if (l2uLeakage) {
    delete[] l2uLeakage;
  }
}

void PerBlockPower::setNumTiles(const uint32_t newNumCoreTiles, const uint32_t newNumL2Tiles) {
  if (corePowers) {
    delete[] corePowers;
  }
  if (l2uDynamic) {
    delete[] l2uDynamic;
  }
  if (l2uLeakage) {
    delete[] l2uLeakage;
  }

  numCoreTiles = newNumCoreTiles;
  numL2Tiles = newNumL2Tiles;
  corePowers = new CoreBlockPowers[numCoreTiles];
  l2uDynamic = new double[numL2Tiles];
  l2uLeakage = new double[numL2Tiles];
  memset(corePowers, 0, numCoreTiles * sizeof(CoreBlockPowers));
  memset(l2uDynamic, 0, numL2Tiles * sizeof(double));
  memset(l2uLeakage, 0, numL2Tiles * sizeof(double));
}

double PerBlockPower::getCoreDynamic(const uint32_t i) const {
  return (corePowers[i].l1iDynamic + corePowers[i].itlbDynamic + corePowers[i].bpredDynamic + corePowers[i].intRenameDynamic +
          corePowers[i].fpRenameDynamic + corePowers[i].intRegfileDynamic + corePowers[i].fpRegfileDynamic + corePowers[i].windowDynamic +
          corePowers[i].intAluDynamic + corePowers[i].fpAddDynamic + corePowers[i].fpMulDynamic + corePowers[i].l1dDynamic +
          corePowers[i].dtlbDynamic + corePowers[i].lsqDynamic + corePowers[i].resultBusDynamic + corePowers[i].clockDynamic);
}

double PerBlockPower::getCoreLeakage(const uint32_t i) const {
  return (corePowers[i].l1iLeakage + corePowers[i].itlbLeakage + corePowers[i].bpredLeakage + corePowers[i].intRenameLeakage +
          corePowers[i].fpRenameLeakage + corePowers[i].intRegfileLeakage + corePowers[i].fpRegfileLeakage + corePowers[i].windowLeakage +
          corePowers[i].intAluLeakage + corePowers[i].fpAddLeakage + corePowers[i].fpMulLeakage + corePowers[i].l1dLeakage +
          corePowers[i].dtlbLeakage + corePowers[i].lsqLeakage + corePowers[i].resultBusLeakage + corePowers[i].clockLeakage);
}

double PerBlockPower::getCorePower(const uint32_t i) const {
  return getCoreDynamic(i) + getCoreLeakage(i);
}

double PerBlockPower::getTotalDynamic() const {
  double toReturn = 0;
  uint32_t i;

  for (i = 0; i < numCoreTiles; ++i) {
    toReturn += (corePowers[i].l1iDynamic + corePowers[i].itlbDynamic + corePowers[i].bpredDynamic + corePowers[i].intRenameDynamic +
                 corePowers[i].fpRenameDynamic + corePowers[i].intRegfileDynamic + corePowers[i].fpRegfileDynamic + corePowers[i].windowDynamic +
                 corePowers[i].intAluDynamic + corePowers[i].fpAddDynamic + corePowers[i].fpMulDynamic + corePowers[i].l1dDynamic +
                 corePowers[i].dtlbDynamic + corePowers[i].lsqDynamic + corePowers[i].resultBusDynamic + corePowers[i].clockDynamic);
  }
  for (i = 0; i < numL2Tiles; ++i) {
    toReturn += l2uDynamic[i];
  }
  return toReturn;
}

double PerBlockPower::getTotalLeakage() const {
  double toReturn = 0;
  uint32_t i;

  for (i = 0; i < numCoreTiles; ++i) {
    toReturn += (corePowers[i].l1iLeakage + corePowers[i].itlbLeakage + corePowers[i].bpredLeakage + corePowers[i].intRenameLeakage +
                 corePowers[i].fpRenameLeakage + corePowers[i].intRegfileLeakage + corePowers[i].fpRegfileLeakage + corePowers[i].windowLeakage +
                 corePowers[i].intAluLeakage + corePowers[i].fpAddLeakage + corePowers[i].fpMulLeakage + corePowers[i].l1dLeakage +
                 corePowers[i].dtlbLeakage + corePowers[i].lsqLeakage + corePowers[i].resultBusLeakage + corePowers[i].clockLeakage);
  }
  for (i = 0; i < numL2Tiles; ++i) {
    toReturn += l2uLeakage[i];
  }
  return toReturn;
}

double PerBlockPower::getTotalPower() const {
  return getTotalDynamic() + getTotalLeakage();
}

PerBlockPower & PerBlockPower::operator=(const PerBlockPower & source) {
  if (this == &source) {
    return *this;
  }

  setNumTiles(source.numCoreTiles, source.numL2Tiles);

  memcpy(corePowers, source.corePowers, numCoreTiles * sizeof(CoreBlockPowers));
  memcpy(l2uDynamic, source.l2uDynamic, numCoreTiles * sizeof(double));
  memcpy(l2uLeakage, source.l2uLeakage, numCoreTiles * sizeof(double));

  return *this;
}

PerBlockPower PerBlockPower::operator-(const PerBlockPower & p2) const {
  PerBlockPower result;
  uint32_t i;

  assert(numCoreTiles == p2.numCoreTiles);
  assert(numL2Tiles == p2.numL2Tiles);
  result.setNumTiles(numCoreTiles, numL2Tiles);

  for (i = 0; i < numCoreTiles; ++i) {
    result.corePowers[i].l1iDynamic = corePowers[i].l1iDynamic - p2.corePowers[i].l1iDynamic;
    result.corePowers[i].itlbDynamic = corePowers[i].itlbDynamic - p2.corePowers[i].itlbDynamic;
    result.corePowers[i].bpredDynamic = corePowers[i].bpredDynamic - p2.corePowers[i].bpredDynamic;
    result.corePowers[i].intRenameDynamic = corePowers[i].intRenameDynamic - p2.corePowers[i].intRenameDynamic;
    result.corePowers[i].fpRenameDynamic = corePowers[i].fpRenameDynamic - p2.corePowers[i].fpRenameDynamic;
    result.corePowers[i].intRegfileDynamic = corePowers[i].intRegfileDynamic - p2.corePowers[i].intRegfileDynamic;
    result.corePowers[i].fpRegfileDynamic = corePowers[i].fpRegfileDynamic - p2.corePowers[i].fpRegfileDynamic;
    result.corePowers[i].windowDynamic = corePowers[i].windowDynamic - p2.corePowers[i].windowDynamic;
    result.corePowers[i].intAluDynamic = corePowers[i].intAluDynamic - p2.corePowers[i].intAluDynamic;
    result.corePowers[i].fpAddDynamic = corePowers[i].fpAddDynamic - p2.corePowers[i].fpAddDynamic;
    result.corePowers[i].fpMulDynamic = corePowers[i].fpMulDynamic - p2.corePowers[i].fpMulDynamic;
    result.corePowers[i].l1dDynamic = corePowers[i].l1dDynamic - p2.corePowers[i].l1dDynamic;
    result.corePowers[i].dtlbDynamic = corePowers[i].dtlbDynamic - p2.corePowers[i].dtlbDynamic;
    result.corePowers[i].lsqDynamic = corePowers[i].lsqDynamic - p2.corePowers[i].lsqDynamic;
    result.corePowers[i].resultBusDynamic = corePowers[i].resultBusDynamic - p2.corePowers[i].resultBusDynamic;
    result.corePowers[i].clockDynamic = corePowers[i].clockDynamic - p2.corePowers[i].clockDynamic;
    result.corePowers[i].l1iLeakage = corePowers[i].l1iLeakage - p2.corePowers[i].l1iLeakage;
    result.corePowers[i].itlbLeakage = corePowers[i].itlbLeakage - p2.corePowers[i].itlbLeakage;
    result.corePowers[i].bpredLeakage = corePowers[i].bpredLeakage - p2.corePowers[i].bpredLeakage;
    result.corePowers[i].intRenameLeakage = corePowers[i].intRenameLeakage - p2.corePowers[i].intRenameLeakage;
    result.corePowers[i].fpRenameLeakage = corePowers[i].fpRenameLeakage - p2.corePowers[i].fpRenameLeakage;
    result.corePowers[i].intRegfileLeakage = corePowers[i].intRegfileLeakage - p2.corePowers[i].intRegfileLeakage;
    result.corePowers[i].fpRegfileLeakage = corePowers[i].fpRegfileLeakage - p2.corePowers[i].fpRegfileLeakage;
    result.corePowers[i].windowLeakage = corePowers[i].windowLeakage - p2.corePowers[i].windowLeakage;
    result.corePowers[i].intAluLeakage = corePowers[i].intAluLeakage - p2.corePowers[i].intAluLeakage;
    result.corePowers[i].fpAddLeakage = corePowers[i].fpAddLeakage - p2.corePowers[i].fpAddLeakage;
    result.corePowers[i].fpMulLeakage = corePowers[i].fpMulLeakage - p2.corePowers[i].fpMulLeakage;
    result.corePowers[i].l1dLeakage = corePowers[i].l1dLeakage - p2.corePowers[i].l1dLeakage;
    result.corePowers[i].dtlbLeakage = corePowers[i].dtlbLeakage - p2.corePowers[i].dtlbLeakage;
    result.corePowers[i].lsqLeakage = corePowers[i].lsqLeakage - p2.corePowers[i].lsqLeakage;
    result.corePowers[i].resultBusLeakage = corePowers[i].resultBusLeakage - p2.corePowers[i].resultBusLeakage;
  }
  for (i = 0; i < numL2Tiles; ++i) {
    result.l2uDynamic[i] = l2uDynamic[i] - p2.l2uDynamic[i];
    result.l2uLeakage[i] = l2uLeakage[i] - p2.l2uLeakage[i];
  }
  return result;
}
