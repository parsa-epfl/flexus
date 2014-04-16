/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

#ifndef FLEXUS_CORE_POWER_COSTS_HPP_INCLUDED
#define FLEXUS_CORE_POWER_COSTS_HPP_INCLUDED

#include <iostream>

class CorePowerCosts {
public:
  CorePowerCosts();
  CorePowerCosts(const CorePowerCosts & source);
  CorePowerCosts & operator=(const CorePowerCosts & source);

private:
  // Parameters
  double vdd,
         frq;
  double baseVdd; // Baseline VDD - use by CACTI in computing cache configurations
  int32_t decodeWidth,
          issueWidth,
          commitWidth,
          windowSize,
          numIntPhysicalRegisters,
          numFpPhysicalRegisters,
          numIntArchitecturalRegs,
          numFpArchitecturalRegs,
          lsqSize,
          dataPathWidth,
          instructionLength;

  int32_t virtualAddressSize;

  // Functional units
  int32_t resIntAlu, // Includes ALUs and multipliers because Wattch cannot differentiate
          resFpAdd,
          resFpMult,
          resMemPort;

  // Branch predictor parameters
  int32_t btbNumSets,
          btbAssociativity,
          twoLevelL1Size,
          twoLevelL2Size,
          twoLevelHistorySize,
          bimodalTableSize,
          metaTableSize,
          rasSize;

  // Cache parameters
  int l1iNumSets,   // L1I
      l1iAssociativity,
      l1iBlockSize,
      l1dNumSets,   // L1D
      l1dAssociativity,
      l1dBlockSize;

  // TLB parameters
  int itlbNumSets,  // ITLB
      itlbAssociativity,
      itlbPageSize,
      dtlbNumSets,  // DTLB
      dtlbAssociativity,
      dtlbPageSize;

  int32_t systemWidth;

  double clockCap;

  // Per-access dynamic power
  double l1iPerAccessDynamic,
         itlbPerAccessDynamic,
         intRenamePerAccessDynamic,
         fpRenamePerAccessDynamic,
         bpredPerAccessDynamic,
         intRegFilePerAccessDynamic,
         fpRegFilePerAccessDynamic,
         resultBusPerAccessDynamic,
         selectionPerAccessDynamic,
         rsPerAccessDynamic,
         wakeupPerAccessDynamic,
         intAluPerAccessDynamic,
         fpAluPerAccessDynamic,
         fpAddPerAccessDynamic,
         fpMulPerAccessDynamic,
         lsqWakeupPerAccessDynamic,
         lsqRsPerAccessDynamic,
         l1dPerAccessDynamic,
         dtlbPerAccessDynamic,
         clockPerCycleDynamic,
         maxCycleDynamic;

public:
  // Setters for parameters
  inline void setVdd(const double newVdd) {
    vdd = newVdd;
  }
  inline void setBaseVdd(const double newVdd) {
    baseVdd = newVdd;
  }
  inline void setFrq(const double newFrq) {
    frq = newFrq;
  }
  inline void setDecodeWidth(const int32_t newDecodeWidth) {
    decodeWidth = newDecodeWidth;
  }
  inline void setIssueWidth(const int32_t newIssueWidth) {
    issueWidth = newIssueWidth;
  }
  inline void setCommitWidth(const int32_t newCommitWidth) {
    commitWidth = newCommitWidth;
  }
  inline void setWindowSize(const int32_t newWindowSize) {
    windowSize = newWindowSize;
  }
  inline void setNumIntPhysicalRegisters(const int32_t newNumIntPhysicalRegisters) {
    numIntPhysicalRegisters = newNumIntPhysicalRegisters;
  }
  inline void setNumFpPhysicalRegisters(const int32_t newNumFpPhysicalRegisters) {
    numFpPhysicalRegisters = newNumFpPhysicalRegisters;
  }
  inline void setNumIntArchitecturalRegs(const int32_t newNumIntArchitecturalRegs) {
    numIntArchitecturalRegs = newNumIntArchitecturalRegs;
  }
  inline void setNumFpArchitecturalRegs(const int32_t newNumFpArchitecturalRegs) {
    numFpArchitecturalRegs = newNumFpArchitecturalRegs;
  }
  inline void setLsqSize(const int32_t newLsqSize) {
    lsqSize = newLsqSize;
  }
  inline void setDataPathWidth(const int32_t newDataPathWidth) {
    dataPathWidth = newDataPathWidth;
  }
  inline void setInstructionLength(const int32_t newInstructionLength) {
    instructionLength = newInstructionLength;
  }
  inline void setVirtualAddressSize(const int32_t newVirtualAddressSize) {
    virtualAddressSize = newVirtualAddressSize;
  }
  inline void setResIntAlu(const int32_t newResIntAlu) {
    resIntAlu = newResIntAlu;
  }
  inline void setResFpAdd(const int32_t newResFpAdd) {
    resFpAdd = newResFpAdd;
  }
  inline void setResFpMult(const int32_t newResFpMult) {
    resFpMult = newResFpMult;
  }
  inline void setResMemPort(const int32_t newResMemPort) {
    resMemPort = newResMemPort;
  }
  inline void setBtbNumSets(const int32_t newBtbNumSets) {
    btbNumSets = newBtbNumSets;
  }
  inline void setBtbAssociativity(const int32_t newBtbAssociativity) {
    btbAssociativity = newBtbAssociativity;
  }
  inline void setTwoLevelL1Size(const int32_t newTwoLevelL1Size) {
    twoLevelL1Size = newTwoLevelL1Size;
  }
  inline void setTwoLevelL2Size(const int32_t newTwoLevelL2Size) {
    twoLevelL2Size = newTwoLevelL2Size;
  }
  inline void setTwoLevelHistorySize(const int32_t newTwoLevelHistorySize) {
    twoLevelHistorySize = newTwoLevelHistorySize;
  }
  inline void setBimodalTableSize(const int32_t newBimodalTableSize) {
    bimodalTableSize = newBimodalTableSize;
  }
  inline void setMetaTableSize(const int32_t newMetaTableSize) {
    metaTableSize = newMetaTableSize;
  }
  inline void setRasSize(const int32_t newRasSize) {
    rasSize = newRasSize;
  }
  inline void setL1iNumSets(const int32_t newL1iNumSets) {
    l1iNumSets = newL1iNumSets;
  }
  inline void setL1iAssociativity(const int32_t newL1iAssociativity) {
    l1iAssociativity = newL1iAssociativity;
  }
  inline void setL1iBlockSize(const int32_t newL1iBlockSize) {
    l1iBlockSize = newL1iBlockSize;
  }
  inline void setL1dNumSets(const int32_t newL1dNumSets) {
    l1dNumSets = newL1dNumSets;
  }
  inline void setL1dAssociativity(const int32_t newL1dAssociativity) {
    l1dAssociativity = newL1dAssociativity;
  }
  inline void setL1dBlockSize(const int32_t newL1dBlockSize) {
    l1dBlockSize = newL1dBlockSize;
  }
  inline void setItlbNumSets(const int32_t newItlbNumSets) {
    itlbNumSets = newItlbNumSets;
  }
  inline void setItlbAssociativity(const int32_t newItlbAssociativity) {
    itlbAssociativity = newItlbAssociativity;
  }
  inline void setItlbPageSize(const int32_t newItlbPageSize) {
    itlbPageSize = newItlbPageSize;
  }
  inline void setDtlbNumSets(const int32_t newDtlbNumSets) {
    dtlbNumSets = newDtlbNumSets;
  }
  inline void setDtlbAssociativity(const int32_t newDtlbAssociativity) {
    dtlbAssociativity = newDtlbAssociativity;
  }
  inline void setDtlbPageSize(const int32_t newDtlbPageSize) {
    dtlbPageSize = newDtlbPageSize;
  }
  inline void setSystemWidth(const int32_t newSystemWidth) {
    systemWidth = newSystemWidth;
  }

  // Getters for parameters
  inline double getVdd() const {
    return vdd;
  }
  inline double getBaseVdd() const {
    return baseVdd;
  }
  inline double getFrq() const {
    return frq;
  }
  inline int32_t getDecodeWidth() const {
    return decodeWidth;
  }
  inline int32_t getIssueWidth() const {
    return issueWidth;
  }
  inline int32_t getCommitWidth() const {
    return commitWidth;
  }
  inline int32_t getWindowSize() const {
    return windowSize;
  }
  inline int32_t getNumIntPhysicalRegisters() const {
    return numIntPhysicalRegisters;
  }
  inline int32_t getNumFpPhysicalRegisters() const {
    return numFpPhysicalRegisters;
  }
  inline int32_t getNumIntArchitecturalRegs() const {
    return numIntArchitecturalRegs;
  }
  inline int32_t getNumFpArchitecturalRegs() const {
    return numFpArchitecturalRegs;
  }
  inline int32_t getLsqSize() const {
    return lsqSize;
  }
  inline int32_t getDataPathWidth() const {
    return dataPathWidth;
  }
  inline int32_t getInstructionLength() const {
    return instructionLength;
  }
  inline int32_t getVirtualAddressSize() const {
    return virtualAddressSize;
  }
  inline int32_t getResIntAlu() const {
    return resIntAlu;
  }
  inline int32_t getResFpAdd() const {
    return resFpAdd;
  }
  inline int32_t getResFpMult() const {
    return resFpMult;
  }
  inline int32_t getResMemPort() const {
    return resMemPort;
  }
  inline int32_t getBtbNumSets() const {
    return btbNumSets;
  }
  inline int32_t getBtbAssociativity() const {
    return btbAssociativity;
  }
  inline int32_t getTwoLevelL1Size() const {
    return twoLevelL1Size;
  }
  inline int32_t getTwoLevelL2Size() const {
    return twoLevelL2Size;
  }
  inline int32_t getTwoLevelHistorySize() const {
    return twoLevelHistorySize;
  }
  inline int32_t getBimodalTableSize() const {
    return bimodalTableSize;
  }
  inline int32_t getMetaTableSize() const {
    return metaTableSize;
  }
  inline int32_t getRasSize() const {
    return rasSize;
  }
  inline int32_t getL1iNumSets() const {
    return l1iNumSets;
  }
  inline int32_t getL1iAssociativity() const {
    return l1iAssociativity;
  }
  inline int32_t getL1iBlockSize() const {
    return l1iBlockSize;
  }
  inline int32_t getL1dNumSets() const {
    return l1dNumSets;
  }
  inline int32_t getL1dAssociativity() const {
    return l1dAssociativity;
  }
  inline int32_t getL1dBlockSize() const {
    return l1dBlockSize;
  }
  inline int32_t getItlbNumSets() const {
    return itlbNumSets;
  }
  inline int32_t getItlbAssociativity() const {
    return itlbAssociativity;
  }
  inline int32_t getItlbPageSize() const {
    return itlbPageSize;
  }
  inline int32_t getDtlbNumSets() const {
    return dtlbNumSets;
  }
  inline int32_t getDtlbAssociativity() const {
    return dtlbAssociativity;
  }
  inline int32_t getDtlbPageSize() const {
    return dtlbPageSize;
  }
  inline int32_t getSystemWidth() const {
    return systemWidth;
  }

  // Getters for power costs
  inline double getL1iPerAccessDynamic() const {
    return l1iPerAccessDynamic;
  }
  inline double getItlbPerAccessDynamic() const {
    return itlbPerAccessDynamic;
  }
  inline double getIntRenamePerAccessDynamic() const {
    return intRenamePerAccessDynamic;
  }
  inline double getFpRenamePerAccessDynamic() const {
    return fpRenamePerAccessDynamic;
  }
  inline double getBpredPerAccessDynamic() const {
    return bpredPerAccessDynamic;
  }
  inline double getIntRegFilePerAccessDynamic() const {
    return intRegFilePerAccessDynamic;
  }
  inline double getFpRegFilePerAccessDynamic() const {
    return fpRegFilePerAccessDynamic;
  }
  inline double getResultBusPerAccessDynamic() const {
    return resultBusPerAccessDynamic;
  }
  inline double getSelectionPerAccessDynamic() const {
    return selectionPerAccessDynamic;
  }
  inline double getRsPerAccessDynamic() const {
    return rsPerAccessDynamic;
  }
  inline double getWakeupPerAccessDynamic() const {
    return wakeupPerAccessDynamic;
  }
  inline double getIntAluPerAccessDynamic() const {
    return intAluPerAccessDynamic;
  }
  inline double getFpAluPerAccessDynamic() const {
    return fpAluPerAccessDynamic;
  }
  inline double getFpAddPerAccessDynamic() const {
    return fpAddPerAccessDynamic;
  }
  inline double getFpMulPerAccessDynamic() const {
    return fpMulPerAccessDynamic;
  }
  inline double getLsqWakeupPerAccessDynamic() const {
    return lsqWakeupPerAccessDynamic;
  }
  inline double getLsqRsPerAccessDynamic() const {
    return lsqRsPerAccessDynamic;
  }
  inline double getL1dPerAccessDynamic() const {
    return l1dPerAccessDynamic;
  }
  inline double getDtlbPerAccessDynamic() const {
    return dtlbPerAccessDynamic;
  }
  inline double getClockPerCycleDynamic() const {
    return clockPerCycleDynamic;
  }
  inline double getMaxCycleDynamic() const {
    return maxCycleDynamic;
  }

  // Calculate the power costs
  void computeAccessCosts();

private:
  void calculate_core_power();
  double perCoreClockPower(const double tile_width, const double tile_height, const double die_width, const double die_height);
};
#endif
