#include "L2PowerCosts.hpp"
#include "WattchFunctions.hpp"
#include "def.hpp"

double calculateL2AccessCost(const double frq, const double vdd, const int32_t numMemFus, const int32_t l2uNumSets, const int32_t l2uBlockSize, const int32_t l2uAssociativity, const int32_t virtualAddressSize) {
  double predeclength, wordlinelength, bitlinelength;
  int32_t ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c, b, a, rowsb, colsb;
  int32_t trowsb, tcolsb, tagsize;
  double dcache2_decoder,
         dcache2_wordline,
         dcache2_bitline,
         dcache2_senseamp,
         dcache2_tagarray;
  double result;
  double clockCap;

  /* these variables are needed to use Cacti to auto-size cache arrays (for optimal delay) */
  cacti_result_type time_result;
  cacti_parameter_type time_parameters;

  time_parameters.cache_size = l2uNumSets * l2uBlockSize * l2uAssociativity; /* C */
  time_parameters.block_size = l2uBlockSize; /* B */
  time_parameters.associativity = l2uAssociativity; /* A */
  time_parameters.number_of_sets = l2uNumSets; /* C/(B*A) */

  calculate_time(&time_result, &time_parameters, vdd);

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

  tagsize = virtualAddressSize - ((int)logtwo(l2uNumSets) + (int)logtwo(l2uBlockSize));
  trowsb = c / (b * a * ntbl * ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd / ntwl;

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb * (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  dcache2_decoder = array_decoder_power(rowsb, colsb, predeclength, 1, 1, frq, vdd);
  dcache2_wordline = array_wordline_power(rowsb, colsb, wordlinelength, 1, 1, frq, vdd);
  dcache2_bitline = array_bitline_power(rowsb, colsb, bitlinelength, 1, 1, 1, frq, vdd, clockCap);
  dcache2_senseamp = senseamp_power(colsb, vdd);
  dcache2_tagarray = simple_array_power(trowsb, tcolsb, 1, 1, 1, frq, vdd, clockCap);

  result = dcache2_decoder + dcache2_wordline + dcache2_bitline + dcache2_senseamp + dcache2_tagarray;
  result *= crossover_scaling;
  result /= numMemFus;

  return result;
}

double calculateNiBufferAccessCost(const double frq, const double vdd, const int32_t numEntries, const int32_t entrySize) {
  double clockCap;
  return simple_array_power(numEntries, 8 * entrySize, 1, 1, 0, frq, vdd, clockCap);
}

double perL2ClockPower(double tile_width, double tile_height, double die_width, double die_height, const double frq, const double vdd, uint32_t systemWidth) {
  double l2clocklinelength, dieclocklinelength;
  double Cline, Ctotal;
  double global_buffercap = 0;

  // Clocks within the tile
  l2clocklinelength = 1 * .5 * tile_width + 2 * .5 * tile_height + 4 * .25 * tile_width + 8 * .25 * tile_height + 16 * .125 * tile_width + 32 * .125 * tile_height;

  // H-tree to get global clock there
  dieclocklinelength = (1 * 4 * tile_height + 2 * 2 * tile_width + 4 * 2 * tile_height + 8 * 1 * tile_width + 16 * 1 * tile_height) / static_cast<double>(2 * systemWidth);

  Cline = 20 * Cmetal * (l2clocklinelength + dieclocklinelength) * 1e3; // Times 1e3 to convert from mm to um

  global_buffercap = (12 * gatecap(1000.0, 10.0) + 16 * gatecap(200, 10.0) + 16 * 8 * 2 * gatecap(100.0, 10.00) + 2 * gatecap(.29 * 1e6, 10.0)) / static_cast<double>(2 * systemWidth);

  Ctotal = Cline + (global_buffercap);

  return Ctotal * Powerfactor * 2.28076202;
}
