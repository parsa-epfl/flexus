#include <cstring>
#include <cmath>

#include "WattchFunctions.hpp"
#include "CorePowerCosts.hpp"
#include "def.hpp"

CorePowerCosts::CorePowerCosts() { }

CorePowerCosts::CorePowerCosts(const CorePowerCosts & source) :
  vdd(source.vdd),
  frq(source.frq),
  baseVdd(source.baseVdd),
  decodeWidth(source.decodeWidth),
  issueWidth(source.issueWidth),
  commitWidth(source.commitWidth),
  windowSize(source.windowSize),
  numIntPhysicalRegisters(source.numIntPhysicalRegisters),
  numFpPhysicalRegisters(source.numFpPhysicalRegisters),
  numIntArchitecturalRegs(source.numIntArchitecturalRegs),
  numFpArchitecturalRegs(source.numFpArchitecturalRegs),
  lsqSize(source.lsqSize),
  dataPathWidth(source.dataPathWidth),
  instructionLength(source.instructionLength),
  virtualAddressSize(source.virtualAddressSize),
  resIntAlu(source.resIntAlu),
  resFpAdd(source.resFpAdd),
  resFpMult(source.resFpMult),
  resMemPort(source.resMemPort),
  btbNumSets(source.btbNumSets),
  btbAssociativity(source.btbAssociativity),
  twoLevelL1Size(source.twoLevelL1Size),
  twoLevelL2Size(source.twoLevelL2Size),
  twoLevelHistorySize(source.twoLevelHistorySize),
  bimodalTableSize(source.bimodalTableSize),
  metaTableSize(source.metaTableSize),
  rasSize(source.rasSize),
  l1iNumSets(source.l1iNumSets),
  l1iAssociativity(source.l1iAssociativity),
  l1iBlockSize(source.l1iBlockSize),
  l1dNumSets(source.l1dNumSets),
  l1dAssociativity(source.l1dAssociativity),
  l1dBlockSize(source.l1dBlockSize),
  itlbNumSets(source.itlbNumSets),
  itlbAssociativity(source.itlbAssociativity),
  itlbPageSize(source.itlbPageSize),
  dtlbNumSets(source.dtlbNumSets),
  dtlbAssociativity(source.dtlbAssociativity),
  dtlbPageSize(source.dtlbPageSize),
  systemWidth(source.systemWidth),
  clockCap(source.clockCap),
  l1iPerAccessDynamic(source.l1iPerAccessDynamic),
  itlbPerAccessDynamic(source.itlbPerAccessDynamic),
  intRenamePerAccessDynamic(source.intRenamePerAccessDynamic),
  fpRenamePerAccessDynamic(source.fpRenamePerAccessDynamic),
  bpredPerAccessDynamic(source.bpredPerAccessDynamic),
  intRegFilePerAccessDynamic(source.intRegFilePerAccessDynamic),
  fpRegFilePerAccessDynamic(source.fpRegFilePerAccessDynamic),
  resultBusPerAccessDynamic(source.resultBusPerAccessDynamic),
  selectionPerAccessDynamic(source.selectionPerAccessDynamic),
  rsPerAccessDynamic(source.rsPerAccessDynamic),
  wakeupPerAccessDynamic(source.wakeupPerAccessDynamic),
  intAluPerAccessDynamic(source.intAluPerAccessDynamic),
  fpAluPerAccessDynamic(source.fpAluPerAccessDynamic),
  fpAddPerAccessDynamic(source.fpAddPerAccessDynamic),
  fpMulPerAccessDynamic(source.fpMulPerAccessDynamic),
  lsqWakeupPerAccessDynamic(source.lsqWakeupPerAccessDynamic),
  lsqRsPerAccessDynamic(source.lsqRsPerAccessDynamic),
  l1dPerAccessDynamic(source.l1dPerAccessDynamic),
  dtlbPerAccessDynamic(source.dtlbPerAccessDynamic),
  clockPerCycleDynamic(source.clockPerCycleDynamic),
  maxCycleDynamic(source.maxCycleDynamic)
{ }

CorePowerCosts & CorePowerCosts::operator=(const CorePowerCosts & source) {
  vdd = source.vdd;
  frq = source.frq;
  baseVdd = source.baseVdd;
  decodeWidth = source.decodeWidth;
  issueWidth = source.issueWidth;
  commitWidth = source.commitWidth;
  windowSize = source.windowSize;
  numIntPhysicalRegisters = source.numIntPhysicalRegisters;
  numFpPhysicalRegisters = source.numFpPhysicalRegisters;
  numIntArchitecturalRegs = source.numIntArchitecturalRegs;
  numFpArchitecturalRegs = source.numFpArchitecturalRegs;
  lsqSize = source.lsqSize;
  dataPathWidth = source.dataPathWidth;
  instructionLength = source.instructionLength;
  virtualAddressSize = source.virtualAddressSize;
  resIntAlu = source.resIntAlu;
  resFpAdd = source.resFpAdd;
  resFpMult = source.resFpMult;
  resMemPort = source.resMemPort;
  btbNumSets = source.btbNumSets;
  btbAssociativity = source.btbAssociativity;
  twoLevelL1Size = source.twoLevelL1Size;
  twoLevelL2Size = source.twoLevelL2Size;
  twoLevelHistorySize = source.twoLevelHistorySize;
  bimodalTableSize = source.bimodalTableSize;
  metaTableSize = source.metaTableSize;
  rasSize = source.rasSize;
  l1iNumSets = source.l1iNumSets;
  l1iAssociativity = source.l1iAssociativity;
  l1iBlockSize = source.l1iBlockSize;
  l1dNumSets = source.l1dNumSets;
  l1dAssociativity = source.l1dAssociativity;
  l1dBlockSize = source.l1dBlockSize;
  itlbNumSets = source.itlbNumSets;
  itlbAssociativity = source.itlbAssociativity;
  itlbPageSize = source.itlbPageSize;
  dtlbNumSets = source.dtlbNumSets;
  dtlbAssociativity = source.dtlbAssociativity;
  dtlbPageSize = source.dtlbPageSize;
  systemWidth = source.systemWidth;
  clockCap = source.clockCap;
  l1iPerAccessDynamic = source.l1iPerAccessDynamic;
  itlbPerAccessDynamic = source.itlbPerAccessDynamic;
  intRenamePerAccessDynamic = source.intRenamePerAccessDynamic;
  fpRenamePerAccessDynamic = source.fpRenamePerAccessDynamic;
  bpredPerAccessDynamic = source.bpredPerAccessDynamic;
  intRegFilePerAccessDynamic = source.intRegFilePerAccessDynamic;
  fpRegFilePerAccessDynamic = source.fpRegFilePerAccessDynamic;
  resultBusPerAccessDynamic = source.resultBusPerAccessDynamic;
  selectionPerAccessDynamic = source.selectionPerAccessDynamic;
  rsPerAccessDynamic = source.rsPerAccessDynamic;
  wakeupPerAccessDynamic = source.wakeupPerAccessDynamic;
  intAluPerAccessDynamic = source.intAluPerAccessDynamic;
  fpAluPerAccessDynamic = source.fpAluPerAccessDynamic;
  fpAddPerAccessDynamic = source.fpAddPerAccessDynamic;
  fpMulPerAccessDynamic = source.fpMulPerAccessDynamic;
  lsqWakeupPerAccessDynamic = source.lsqWakeupPerAccessDynamic;
  lsqRsPerAccessDynamic = source.lsqRsPerAccessDynamic;
  l1dPerAccessDynamic = source.l1dPerAccessDynamic;
  dtlbPerAccessDynamic = source.dtlbPerAccessDynamic;
  clockPerCycleDynamic = source.clockPerCycleDynamic;
  maxCycleDynamic = source.maxCycleDynamic;

  return *this;
}

void CorePowerCosts::computeAccessCosts() {
  calculate_core_power();
  maxCycleDynamic =  l1iPerAccessDynamic +
                     itlbPerAccessDynamic +
                     intRenamePerAccessDynamic * decodeWidth +
                     fpRenamePerAccessDynamic * decodeWidth  +
                     bpredPerAccessDynamic * 2 * decodeWidth +
                     intRegFilePerAccessDynamic * (3.0 * commitWidth) +
                     fpRegFilePerAccessDynamic * (3.0 * commitWidth) +
                     selectionPerAccessDynamic *  (resIntAlu + resFpAdd + resFpMult + resMemPort) +
                     rsPerAccessDynamic * (2.0 * (resIntAlu + resFpAdd + resFpMult + resMemPort)) +
                     wakeupPerAccessDynamic * (resIntAlu + resFpAdd + resFpMult + resMemPort) +
                     intAluPerAccessDynamic * resIntAlu +
                     fpAddPerAccessDynamic * resFpAdd +
                     fpMulPerAccessDynamic * resFpMult +
                     lsqWakeupPerAccessDynamic * resMemPort +
                     lsqRsPerAccessDynamic * resMemPort +
                     l1dPerAccessDynamic * resMemPort +
                     dtlbPerAccessDynamic * resMemPort +
                     resultBusPerAccessDynamic * issueWidth;
}

double CorePowerCosts::perCoreClockPower(double tile_width, double tile_height, double die_width, double die_height) {
  double coreclocklinelength, dieclocklinelength;
  double Cline, Ctotal;
  double pipereg_clockcap = 0;
  double global_buffercap = 0;
  uint32_t total_piperegs;

  total_piperegs = decodeWidth * (instructionLength/*  + N_THREADS */);
  /* pipe stages after decode */
  total_piperegs += decodeWidth * (instructionLength/*  + N_THREADS */);
  /* pipe stages in the reg access clock domain - (2 stages, 2 inter-stage buffers)*/
  total_piperegs += issueWidth * (instructionLength + 2 * dataPathWidth + 3 * 2/*  + N_THREADS */) + commitWidth * (instructionLength + dataPathWidth + 2/*  + N_THREADS */);
  /* Exec clocks */
  total_piperegs += issueWidth * (instructionLength + 2 * dataPathWidth + 3 * 2/*  + N_THREADS */) + commitWidth * (instructionLength + dataPathWidth + 2/*  + N_THREADS */);
  total_piperegs += (issueWidth * (instructionLength + 2 * dataPathWidth + 3 * 2/*  + N_THREADS */) + commitWidth * (instructionLength + dataPathWidth + 2/*  + N_THREADS */)) / 2;
  total_piperegs += (issueWidth * (instructionLength + 2 * dataPathWidth + 3 * 2/*  + N_THREADS */) + commitWidth * (instructionLength + dataPathWidth + 2/*  + N_THREADS */)) / 2;

  pipereg_clockcap = total_piperegs * 4 * gatecap (10.0, 0);

  // Clocks within the tile
  coreclocklinelength = 1 * .5 * tile_width + 2 * .5 * tile_height + 4 * .25 * tile_width + 8 * .25 * tile_height + 16 * .125 * tile_width + 32 * .125 * tile_height;

  // H-tree to get global clock there
  dieclocklinelength = (1 * 4 * tile_height + 2 * 2 * tile_width + 4 * 2 * tile_height + 8 * 1 * tile_width + 16 * 1 * tile_height) / static_cast<double>(2 * systemWidth);

  Cline = 20 * Cmetal * (coreclocklinelength + dieclocklinelength) * 1e3; // Times 1e3 to convert from mm to um

  global_buffercap = (12 * gatecap(1000.0, 10.0) + 16 * gatecap(200, 10.0) + 16 * 8 * 2 * gatecap(100.0, 10.00) + 2 * gatecap(.29 * 1e6, 10.0)) / static_cast<double>(2 * systemWidth);

  Ctotal = Cline + (global_buffercap + pipereg_clockcap + clockCap);

  return (Ctotal * Powerfactor) * 2.28076202; //+ (resIntAlu*I_ADD_CLOCK + resFpAdd*F_ADD_CLOCK + resFpMult * F_MULT_CLOCK));
}

void CorePowerCosts::calculate_core_power() {
  double predeclength, wordlinelength, bitlinelength;
  int32_t ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c, b, a, cache, rowsb, colsb;
  int32_t trowsb, tcolsb, tagsize;
  double rat_decoder,
         rat_wordline,
         rat_bitline,
         rat_senseamp,
         rat_power,
         dcl_compare,
         dcl_pencode,
         dcl_power,
         inst_decoder_power,
         wakeup_tagdrive,
         wakeup_tagmatch,
         wakeup_ormatch,
         int_regfile_decoder,
         int_regfile_wordline,
         int_regfile_bitline,
         int_regfile_senseamp,
         fp_regfile_decoder,
         fp_regfile_wordline,
         fp_regfile_bitline,
         fp_regfile_senseamp,
         rs_decoder,
         rs_wordline,
         rs_bitline,
         rs_senseamp,
         lsq_wakeup_tagdrive,
         lsq_wakeup_tagmatch,
         lsq_wakeup_ormatch,
         lsq_rs_decoder,
         lsq_rs_wordline,
         lsq_rs_bitline,
         lsq_rs_senseamp,
         btb,
         local_predict,
         global_predict,
         chooser,
         ras,
         icache_decoder,
         icache_wordline,
         icache_bitline,
         icache_senseamp,
         icache_tagarray,
         dcache_decoder,
         dcache_wordline,
         dcache_bitline,
         dcache_senseamp,
         dcache_tagarray;
  int32_t npreg_width,
          nvreg_width;
  int32_t numPhysicalRegisters;
  int numIntFus,
      numFpFus,
      numMemFus,
      numFus;

  /* these variables are needed to use Cacti to auto-size cache arrays (for optimal delay) */
  cacti_result_type time_result;
  cacti_parameter_type time_parameters;

  /* used to autosize other structures, like bpred tables */
  int32_t scale_factor;

  numPhysicalRegisters = numIntPhysicalRegisters + numFpPhysicalRegisters;
  numIntFus = resIntAlu;
  numFpFus = resFpAdd + resFpMult;
  numMemFus = resMemPort;
  numFus = numIntFus + numFpFus + numMemFus;

  npreg_width = static_cast<int>(ceil(logtwo(static_cast<double>(numPhysicalRegisters))));

  clockCap = 0.0;

  cache = 0;

  /* FIXME: ALU power is a simple constant, it would be better
    to include bit AFs and have different numbers for different
    types of operations */
// intAluPerAccessDynamic = resIntAlu * I_ADD;
// fpAddPerAccessDynamic = resFpAdd * F_ADD;
// fpMulPerAccessDynamic = resFpMult * F_MULT;

  // Just calculate cost for a single access
  intAluPerAccessDynamic = I_ADD;
  fpAddPerAccessDynamic = F_ADD;
  fpMulPerAccessDynamic = F_MULT;

  nvreg_width = static_cast<int>(ceil(logtwo(static_cast<double>(numIntArchitecturalRegs))));
  npreg_width = (int)ceil(logtwo((double)numPhysicalRegisters));

  /* RAT has shadow bits stored in each cell, this makes the
    cell size larger than normal array structures, so we must
    compute it here */

  predeclength = numIntArchitecturalRegs * (RatCellHeight + 7 * decodeWidth * WordlineSpacing);

  wordlinelength = (7 * (RegCellWidth + 6 * issueWidth * BitlineSpacing));
  bitlinelength = numIntArchitecturalRegs * (RatCellHeight + 3 * decodeWidth * WordlineSpacing);

  rat_decoder = array_decoder_power(numIntArchitecturalRegs, npreg_width, predeclength, 2 * decodeWidth, decodeWidth, frq, vdd);
  rat_wordline = array_wordline_power(numIntArchitecturalRegs, npreg_width, wordlinelength, 2 * decodeWidth, decodeWidth, frq, vdd);
  rat_bitline = array_bitline_power(numIntArchitecturalRegs, npreg_width, bitlinelength, 2 * decodeWidth, decodeWidth, cache, frq, vdd, clockCap);
  rat_senseamp = 0;

  dcl_compare = dcl_compare_power(nvreg_width, decodeWidth, frq, vdd);
  dcl_pencode = 0;

  npreg_width = static_cast<int>(ceil(logtwo(static_cast<double>(numPhysicalRegisters))));
  inst_decoder_power = decodeWidth * simple_array_decoder_power(opcode_length, 1, 1, 1, frq, vdd);

  npreg_width = static_cast<int>(ceil(logtwo(static_cast<double>(windowSize))));
  wakeup_tagdrive = cam_tagdrive(windowSize, npreg_width, (numIntFus + numFpFus), (numIntFus + numFpFus), frq, vdd);
  wakeup_tagmatch = cam_tagmatch(windowSize, npreg_width, (numIntFus + numFpFus), (numIntFus + numFpFus), frq, vdd, clockCap, issueWidth);

  wakeup_ormatch = 0;

  selectionPerAccessDynamic = selection_power(windowSize, frq, vdd, issueWidth) / numFus;

  predeclength = numIntArchitecturalRegs * (RegCellHeight + 3 * issueWidth * WordlineSpacing);

  wordlinelength = ((dataPathWidth + 5) * (RegCellWidth + 6 * issueWidth * BitlineSpacing));
  bitlinelength = numIntArchitecturalRegs * (RegCellHeight + 3 * issueWidth * WordlineSpacing);

  int_regfile_decoder = array_decoder_power(numIntArchitecturalRegs * 2, dataPathWidth, predeclength, 2 * issueWidth, issueWidth, frq, vdd);
  int_regfile_wordline = array_wordline_power(numIntArchitecturalRegs * 2, dataPathWidth, wordlinelength, 2 * issueWidth, issueWidth, frq, vdd);
  int_regfile_bitline = array_bitline_power(numIntArchitecturalRegs * 2, dataPathWidth, bitlinelength * 3, 2 * issueWidth, issueWidth, cache, frq, vdd, clockCap);
  int_regfile_senseamp = 0;

  wordlinelength = ((dataPathWidth + 5) * (RegCellWidth + 6 * issueWidth * BitlineSpacing));
  bitlinelength = numFpArchitecturalRegs * (RegCellHeight + 3 * issueWidth * WordlineSpacing);

  fp_regfile_decoder = array_decoder_power(numFpArchitecturalRegs * 2, dataPathWidth, predeclength, 2 * issueWidth, issueWidth, frq, vdd);
  fp_regfile_wordline = array_wordline_power(numFpArchitecturalRegs * 2, dataPathWidth, wordlinelength, 2 * issueWidth, issueWidth, frq, vdd);
  fp_regfile_bitline = array_bitline_power(numFpArchitecturalRegs * 2, dataPathWidth, bitlinelength * 3, 2 * issueWidth, issueWidth, cache, frq, vdd, clockCap);
  fp_regfile_senseamp = 0;

  predeclength = windowSize * (RegCellHeight + 3 * (numIntFus + numFpFus) * WordlineSpacing);
  wordlinelength = dataPathWidth * (RegCellWidth + 6 * (numIntFus + numFpFus) * BitlineSpacing);
  bitlinelength = windowSize * (RegCellHeight + 3 * (numIntFus + numFpFus) * WordlineSpacing);

  rs_decoder = array_decoder_power(windowSize, dataPathWidth, predeclength, 2 * (numIntFus + numFpFus), numIntFus + numFpFus, frq, vdd);
  rs_wordline = array_wordline_power(windowSize, dataPathWidth, wordlinelength, 2 * (numIntFus + numFpFus), numIntFus + numFpFus, frq, vdd);
  rs_bitline = array_bitline_power(windowSize, dataPathWidth, bitlinelength, 2 * (numIntFus + numFpFus), numIntFus + numFpFus, cache, frq, vdd, clockCap);
  rs_senseamp = 0;

  /* addresses go into lsq tag's */
  lsq_wakeup_tagdrive = cam_tagdrive(lsqSize, dataPathWidth, resMemPort, resMemPort, frq, vdd);
  lsq_wakeup_tagmatch = cam_tagmatch(lsqSize, dataPathWidth, resMemPort, resMemPort, frq, vdd, clockCap, issueWidth);
  lsq_wakeup_ormatch = 0;

  predeclength = lsqSize * (RegCellHeight + 3 * issueWidth * WordlineSpacing);
  wordlinelength = dataPathWidth * (RegCellWidth + 4 * resMemPort * BitlineSpacing);
  bitlinelength = lsqSize * (RegCellHeight + 4 * resMemPort * WordlineSpacing);

  lsq_rs_decoder = array_decoder_power(lsqSize, dataPathWidth, predeclength, resMemPort, resMemPort, frq, vdd);
  lsq_rs_wordline = array_wordline_power(lsqSize, dataPathWidth, wordlinelength, resMemPort, resMemPort, frq, vdd);
  lsq_rs_bitline = array_bitline_power(lsqSize, dataPathWidth, bitlinelength, resMemPort, resMemPort, cache, frq, vdd, clockCap);
  lsq_rs_senseamp = 0;

  resultBusPerAccessDynamic = compute_resultbus_power(frq, vdd, numPhysicalRegisters, numFus, issueWidth, dataPathWidth);

  // Branch predictor BTB
  /* Load cache values into what cacti is expecting */
  time_parameters.cache_size = btbNumSets * (dataPathWidth / 8) * btbAssociativity; /* C */
  time_parameters.block_size = (dataPathWidth / 8); /* B */
  time_parameters.associativity = btbAssociativity; /* A */
  time_parameters.number_of_sets = btbNumSets; /* C/(B*A) */

  /* have Cacti compute optimal cache config */
  calculate_time(&time_result, &time_parameters, baseVdd);

  /* extract Cacti results */
  ndwl = time_result.best_Ndwl;
  ndbl = time_result.best_Ndbl;
  nspd = time_result.best_Nspd;
  ntwl = time_result.best_Ntwl;
  ntbl = time_result.best_Ntbl;
  ntspd = time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  cache = 1;

  /* Figure out how many rows/cols there are now */
  rowsb = c / (b * a * ndbl * nspd);
  colsb = 8 * b * a * nspd / ndwl;

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb * (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  btb = ndwl * ndbl * (array_decoder_power(rowsb, colsb, predeclength, 1, 1, frq, vdd) + array_wordline_power(rowsb, colsb, wordlinelength, 1, 1, frq, vdd) + array_bitline_power(rowsb, colsb, bitlinelength, 1, 1, cache, frq, vdd, clockCap) + senseamp_power(colsb, vdd));

  cache = 1;

// Branch predictor two level
  scale_factor = squarify(twoLevelL1Size, twoLevelHistorySize);
  predeclength = (twoLevelL1Size / scale_factor) * (RegCellHeight + WordlineSpacing);
  wordlinelength = twoLevelHistorySize * scale_factor * (RegCellWidth + BitlineSpacing);
  bitlinelength = (twoLevelL1Size / scale_factor) * (RegCellHeight + WordlineSpacing);

  local_predict = array_decoder_power(twoLevelL1Size / scale_factor, twoLevelHistorySize * scale_factor, predeclength, 1, 1, frq, vdd) + array_wordline_power(twoLevelL1Size / scale_factor, twoLevelHistorySize * scale_factor, wordlinelength, 1, 1, frq, vdd) + array_bitline_power(twoLevelL1Size / scale_factor, twoLevelHistorySize * scale_factor, bitlinelength, 1, 1, cache, frq, vdd, clockCap) + senseamp_power(twoLevelHistorySize * scale_factor, vdd);

  scale_factor = squarify(twoLevelL2Size, 3);

  predeclength = (twoLevelL2Size / scale_factor) * (RegCellHeight + WordlineSpacing);
  wordlinelength = 3 * scale_factor * (RegCellWidth + BitlineSpacing);
  bitlinelength = (twoLevelL2Size / scale_factor) * (RegCellHeight + WordlineSpacing);

  local_predict += array_decoder_power(twoLevelL2Size / scale_factor, 3 * scale_factor, predeclength, 1, 1, frq, vdd) + array_wordline_power(twoLevelL2Size / scale_factor, 3 * scale_factor, wordlinelength, 1, 1, frq, vdd) + array_bitline_power(twoLevelL2Size / scale_factor, 3 * scale_factor, bitlinelength, 1, 1, cache, frq, vdd, clockCap) + senseamp_power(3 * scale_factor, vdd);

// Branch predictor bimodal
  scale_factor = squarify(bimodalTableSize, 2);

  predeclength = bimodalTableSize / scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2 * scale_factor * (RegCellWidth + BitlineSpacing);
  bitlinelength = bimodalTableSize / scale_factor * (RegCellHeight + WordlineSpacing);

  global_predict = array_decoder_power(bimodalTableSize / scale_factor, 2 * scale_factor, predeclength, 1, 1, frq, vdd) + array_wordline_power(bimodalTableSize / scale_factor, 2 * scale_factor, wordlinelength, 1, 1, frq, vdd) + array_bitline_power(bimodalTableSize / scale_factor, 2 * scale_factor, bitlinelength, 1, 1, cache, frq, vdd, clockCap) + senseamp_power(2 * scale_factor, vdd);

// Branch predictor metapredictor
  scale_factor = squarify(metaTableSize, 2);

  predeclength = metaTableSize / scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2 * scale_factor * (RegCellWidth + BitlineSpacing);
  bitlinelength = metaTableSize / scale_factor * (RegCellHeight + WordlineSpacing);

  chooser = array_decoder_power(metaTableSize / scale_factor, 2 * scale_factor, predeclength, 1, 1, frq, vdd) + array_wordline_power(metaTableSize / scale_factor, 2 * scale_factor, wordlinelength, 1, 1, frq, vdd) + array_bitline_power(metaTableSize / scale_factor, 2 * scale_factor, bitlinelength, 1, 1, cache, frq, vdd, clockCap) + senseamp_power(2 * scale_factor, vdd);

// Branch predictor RAS
  ras = simple_array_power(rasSize, dataPathWidth, 1, 1, 0, frq, vdd, clockCap);

// Caches
  tagsize = virtualAddressSize - (static_cast<int>(logtwo(l1dNumSets)) + static_cast<int>(logtwo(l1dBlockSize)));

  dtlbPerAccessDynamic = /*resMemPort**/(cam_array(dtlbNumSets, virtualAddressSize - static_cast<int>(logtwo(static_cast<double>(dtlbPageSize))), 1, 1, frq, vdd, clockCap, issueWidth)) + simple_array_power(dtlbNumSets, tagsize, 1, 1, cache, frq, vdd, clockCap);

  tagsize = virtualAddressSize - (static_cast<int>(logtwo(l1iNumSets)) + static_cast<int>(logtwo(l1iBlockSize)));

  predeclength = itlbNumSets * (RegCellHeight + WordlineSpacing);
  wordlinelength = logtwo(static_cast<double>(itlbPageSize)) * (RegCellWidth + BitlineSpacing);
  bitlinelength = itlbNumSets * (RegCellHeight + WordlineSpacing);

  itlbPerAccessDynamic = cam_array(itlbNumSets, virtualAddressSize - static_cast<int>(logtwo((static_cast<double>(itlbPageSize)))), 1, 1, frq, vdd, clockCap, issueWidth) + simple_array_power(itlbNumSets, tagsize, 1, 1, cache, frq, vdd, clockCap);

  cache = 1;

  time_parameters.cache_size = l1iNumSets * l1iBlockSize * l1iAssociativity; /* C */
  time_parameters.block_size = l1iBlockSize; /* B */
  time_parameters.associativity = l1iAssociativity; /* A */
  time_parameters.number_of_sets = l1iNumSets; /* C/(B*A) */

  calculate_time(&time_result, &time_parameters, baseVdd);

  ndwl = time_result.best_Ndwl;
  ndbl = time_result.best_Ndbl;
  nspd = time_result.best_Nspd;
  ntwl = time_result.best_Ntwl;
  ntbl = time_result.best_Ntbl;
  ntspd = time_result.best_Ntspd;

  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  rowsb = c / (b * a * ndbl * nspd);
  colsb = 8 * b * a * nspd / ndwl;

  tagsize = virtualAddressSize - ((int)logtwo(l1iNumSets) + (int)logtwo(l1iBlockSize));
  trowsb = c / (b * a * ntbl * ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd / ntwl;

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb * (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  icache_decoder = ndwl * ndbl * array_decoder_power(rowsb, colsb, predeclength, 1, 1, frq, vdd);
  icache_wordline = ndwl * ndbl * array_wordline_power(rowsb, colsb, wordlinelength, 1, 1, frq, vdd);
  icache_bitline = ndwl * ndbl * array_bitline_power(rowsb, colsb, bitlinelength, 1, 1, cache, frq, vdd, clockCap);
  icache_senseamp = ndwl * ndbl * senseamp_power(colsb, vdd);
  icache_tagarray = ntwl * ntbl * (simple_array_power(trowsb, tcolsb, 1, 1, cache, frq, vdd, clockCap));

  l1iPerAccessDynamic = icache_decoder + icache_wordline + icache_bitline + icache_senseamp + icache_tagarray;

  time_parameters.cache_size = l1dNumSets * l1dBlockSize * l1dAssociativity; /* C */
  time_parameters.block_size = l1dBlockSize; /* B */
  time_parameters.associativity = l1dAssociativity; /* A */
  time_parameters.number_of_sets = l1dNumSets; /* C/(B*A) */

  calculate_time(&time_result, &time_parameters, baseVdd);

  ndwl = time_result.best_Ndwl;
  ndbl = time_result.best_Ndbl;
  nspd = time_result.best_Nspd;
  ntwl = time_result.best_Ntwl;
  ntbl = time_result.best_Ntbl;
  ntspd = time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  cache = 1;

  rowsb = c / (b * a * ndbl * nspd);
  colsb = 8 * b * a * nspd / ndwl;

  tagsize = virtualAddressSize - ((int)logtwo(l1dNumSets) + (int)logtwo(l1dBlockSize));
  trowsb = c / (b * a * ntbl * ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd / ntwl;

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb * (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  dcache_decoder = resMemPort * ndwl * ndbl * array_decoder_power(rowsb, colsb, predeclength, 1, 1, frq, vdd);
  dcache_wordline = resMemPort * ndwl * ndbl * array_wordline_power(rowsb, colsb, wordlinelength, 1, 1, frq, vdd);
  dcache_bitline = resMemPort * ndwl * ndbl * array_bitline_power(rowsb, colsb, bitlinelength, 1, 1, cache, frq, vdd, clockCap);
  dcache_senseamp = resMemPort * ndwl * ndbl * senseamp_power(colsb, vdd);
  dcache_tagarray = resMemPort * ntwl * ntbl * (simple_array_power(trowsb, tcolsb, 1, 1, cache, frq, vdd, clockCap));

  l1dPerAccessDynamic = dcache_decoder + dcache_wordline + dcache_bitline + dcache_senseamp + dcache_tagarray;

  clockPerCycleDynamic = perCoreClockPower(CORE_WIDTH, CORE_HEIGHT, DIE_WIDTH, DIE_HEIGHT);

  rat_decoder *= crossover_scaling;
  rat_wordline *= crossover_scaling;
  rat_bitline *= crossover_scaling;

  dcl_compare *= crossover_scaling;
  dcl_pencode *= crossover_scaling;
  inst_decoder_power *= crossover_scaling;

  wakeup_tagdrive *= crossover_scaling;
  wakeup_tagmatch *= crossover_scaling;
  wakeup_ormatch *= crossover_scaling;

  selectionPerAccessDynamic *= crossover_scaling;

  int_regfile_decoder *= crossover_scaling;
  int_regfile_wordline *= crossover_scaling;
  int_regfile_bitline *= crossover_scaling;
  int_regfile_senseamp *= crossover_scaling;

  fp_regfile_decoder *= crossover_scaling;
  fp_regfile_wordline *= crossover_scaling;
  fp_regfile_bitline *= crossover_scaling;
  fp_regfile_senseamp *= crossover_scaling;

  rs_decoder *= crossover_scaling;
  rs_wordline *= crossover_scaling;
  rs_bitline *= crossover_scaling;
  rs_senseamp *= crossover_scaling;

  lsq_wakeup_tagdrive *= crossover_scaling;
  lsq_wakeup_tagmatch *= crossover_scaling;

  lsq_rs_decoder *= crossover_scaling;
  lsq_rs_wordline *= crossover_scaling;
  lsq_rs_bitline *= crossover_scaling;
  lsq_rs_senseamp *= crossover_scaling;

  resultBusPerAccessDynamic *= crossover_scaling;

  btb *= crossover_scaling;
  local_predict *= crossover_scaling;
  global_predict *= crossover_scaling;
  chooser *= crossover_scaling;

  dtlbPerAccessDynamic *= crossover_scaling;
  itlbPerAccessDynamic *= crossover_scaling;

  l1iPerAccessDynamic *= crossover_scaling;

  l1dPerAccessDynamic *= crossover_scaling;
  l1dPerAccessDynamic /= numMemFus;

  bpredPerAccessDynamic = (btb + local_predict + global_predict + chooser + ras) / 2.0;

  rat_power = rat_decoder + rat_wordline + rat_bitline + rat_senseamp;

  dcl_power = dcl_compare + dcl_pencode;

  intRenamePerAccessDynamic = (rat_power + dcl_power + inst_decoder_power) / decodeWidth;
  fpRenamePerAccessDynamic = (rat_power + dcl_power + inst_decoder_power) / decodeWidth;

  wakeupPerAccessDynamic = (wakeup_tagdrive + wakeup_tagmatch + wakeup_ormatch) / (numIntFus + numFpFus + numMemFus);
  rsPerAccessDynamic = (rs_decoder + rs_wordline + rs_bitline + rs_senseamp) / (2.0 * (numIntFus + numFpFus + numMemFus));
  //rsNoBitPerAccessDynamic = rs_decoder + rs_wordline + rs_senseamp;
  //windowPerAccessDynamic = wakeupPerAccessDynamic + rsPerAccessDynamic + selectionPerAccessDynamic;

  lsqRsPerAccessDynamic = (lsq_rs_decoder + lsq_rs_wordline + lsq_rs_bitline + lsq_rs_senseamp) / numMemFus;
  lsqRsPerAccessDynamic = (lsq_rs_decoder + lsq_rs_wordline + lsq_rs_senseamp) / numMemFus;
  lsqWakeupPerAccessDynamic = (lsq_wakeup_tagdrive + lsq_wakeup_tagmatch) / numMemFus;

  intRegFilePerAccessDynamic = (int_regfile_decoder + int_regfile_wordline + int_regfile_bitline + int_regfile_senseamp) / (3.0 * commitWidth);
  //intRegFileNoBitPerAccessDynamic = int_regfile_decoder + int_regfile_wordline + int_regfile_senseamp;

  fpRegFilePerAccessDynamic = (fp_regfile_decoder + fp_regfile_wordline + fp_regfile_bitline + fp_regfile_senseamp) / (3.0 * commitWidth);;
  //fpRegFileNoBitPerAccessDynamic = fp_regfile_decoder + fp_regfile_wordline + fp_regfile_senseamp;
}
