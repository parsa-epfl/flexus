// The TemperatureTracker provides an interface to HotSpot, which is available
// at http://lava.cs.virginia.edu/HotSpot. The HotSpot license is available in
// components/PowerTracker/HotSpot/LICENSE.

#include "TemperatureTracker.hpp"
#include <sstream>
#include <algorithm>
#include <string.h>

TemperatureTracker::~TemperatureTracker() {
  free_flp(flp, 0);
  delete_RC_model(model);
  free_dvector(hs_temp);
  free_dvector(hs_power);
  free_dvector(steady_temp);
  free_dvector(overall_power);
  free_dvector(avg_power);
}

void TemperatureTracker::writeSteadyStateTemperatures() {
  int32_t i;

  if (enabled && strcmp(model->config->steady_file, NULLFILE)) {
    // Find the average per cycle power dissipated
    // Avoid a divide by zero if the user exits the simulator right after starting it
    cycles = std::max(cycles, 1ul);

    for (i = 0; i < flp->n_units; ++i) {
      avg_power[i] = overall_power[i] / static_cast<double>(cycles);
    }

    steady_state_temp(model, avg_power, steady_temp);
    dump_temp(model, steady_temp, model->config->steady_file);
  }
}

void TemperatureTracker::initialize(thermal_config_t & config, const std::string floorplanFile, const uint32_t interval, const uint32_t aNumCoreTiles, const uint32_t aNumL2Tiles) {
  double icacheArea,
         dcacheArea,
         bpredArea,
         dtbArea,
         fpAddArea,
         fpRegArea,
         fpMulArea,
         fpMapArea,
         intMapArea,
         windowArea,
         intRegArea,
         intExecArea,
         ldStQArea,
         itbArea,
         totalArea;

  // initialize flp, get adjacency matrix
  flp = read_flp(const_cast<char *>(floorplanFile.c_str()), FALSE);

  // Allocate and initialize the RC model
  model = alloc_RC_model(&config, flp);
  populate_R_model(model, flp);
  populate_C_model(model, flp);

  // Allocate the temp and power arrays
  // Using hotspot_vector to internally allocate any extra nodes needed
  hs_temp = hotspot_vector(model);
  hs_power = hotspot_vector(model);
  steady_temp = hotspot_vector(model);
  overall_power = hotspot_vector(model);
  avg_power = hotspot_vector(model);

  // Set up initial instantaneous temperatures
  if (strcmp(model->config->init_file, NULLFILE)) {
    if (!model->config->dtm_used) { // initial T = steady T for no DTM
      read_temp(model, hs_temp, model->config->init_file, FALSE);
    } else { // Initial T = clipped steady T with DTM
      read_temp(model, hs_temp, model->config->init_file, TRUE);
    }
  } else { // no input file - use init_temp as the common temperature
    set_temp(model, hs_temp, model->config->init_temp);
  }

  // Calculate area proportions for all blocks in the core
  icacheArea = flp->units[get_blk_index(flp, "0-Icache")].width * flp->units[get_blk_index(flp, "0-Icache")].height;
  dcacheArea = flp->units[get_blk_index(flp, "0-Dcache")].width * flp->units[get_blk_index(flp, "0-Dcache")].height;
  bpredArea = flp->units[get_blk_index(flp, "0-Bpred")].width * flp->units[get_blk_index(flp, "0-Bpred")].height;
  dtbArea = flp->units[get_blk_index(flp, "0-DTB")].width * flp->units[get_blk_index(flp, "0-DTB")].height;
  fpAddArea = flp->units[get_blk_index(flp, "0-FPAdd")].width * flp->units[get_blk_index(flp, "0-FPAdd")].height;
  fpRegArea = flp->units[get_blk_index(flp, "0-FPReg")].width * flp->units[get_blk_index(flp, "0-FPReg")].height;
  fpMulArea = flp->units[get_blk_index(flp, "0-FPMul")].width * flp->units[get_blk_index(flp, "0-FPMul")].height;
  fpMapArea = flp->units[get_blk_index(flp, "0-FPMap")].width * flp->units[get_blk_index(flp, "0-FPMap")].height;
  intMapArea = flp->units[get_blk_index(flp, "0-IntMap")].width * flp->units[get_blk_index(flp, "0-IntMap")].height;
  windowArea = flp->units[get_blk_index(flp, "0-Window")].width * flp->units[get_blk_index(flp, "0-Window")].height;
  intRegArea = flp->units[get_blk_index(flp, "0-IntReg")].width * flp->units[get_blk_index(flp, "0-IntReg")].height;
  intExecArea = flp->units[get_blk_index(flp, "0-IntExec")].width * flp->units[get_blk_index(flp, "0-IntExec")].height;
  ldStQArea = flp->units[get_blk_index(flp, "0-LdStQ")].width * flp->units[get_blk_index(flp, "0-LdStQ")].height;
  itbArea = flp->units[get_blk_index(flp, "0-ITB")].width * flp->units[get_blk_index(flp, "0-ITB")].height;
  totalArea = icacheArea + dcacheArea + bpredArea + dtbArea + fpAddArea + fpRegArea + fpMulArea + fpMapArea + intMapArea + windowArea + intRegArea + intExecArea + ldStQArea + itbArea;

  icacheAreaProportion = icacheArea / totalArea;
  dcacheAreaProportion = dcacheArea / totalArea;
  bpredAreaProportion = bpredArea / totalArea;
  dtbAreaProportion = dtbArea / totalArea;
  fpAddAreaProportion = fpAddArea / totalArea;
  fpRegAreaProportion = fpRegArea / totalArea;
  fpMulAreaProportion = fpMulArea / totalArea;
  fpMapAreaProportion = fpMapArea / totalArea;
  intMapAreaProportion = intMapArea / totalArea;
  windowAreaProportion = windowArea / totalArea;
  intRegAreaProportion = intRegArea / totalArea;
  intExecAreaProportion = intExecArea / totalArea;
  ldStQAreaProportion = ldStQArea / totalArea;
  itbAreaProportion = itbArea / totalArea;

  numCoreTiles = aNumCoreTiles;
  numL2Tiles = aNumL2Tiles;
  cycleInterval = interval;

  cycles = 0;

  enabled = true;
}

void TemperatureTracker::updateTemperature(const PerBlockPower & intervalPower) {
  std::string index;
  std::ostringstream ossIndex;
  uint32_t i;
  int32_t k;

  for (i = 0; i < numCoreTiles; ++i) {
    ossIndex.str("");
    ossIndex << i;
    index = ossIndex.str();

    hs_power[get_blk_index(flp, const_cast<char *>((index + "-Icache").c_str()))] =  intervalPower.getL1iPower(i) + intervalPower.getClockPower(i) * icacheAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-ITB").c_str()))] =  intervalPower.getItlbPower(i) + intervalPower.getClockPower(i) * itbAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-Bpred").c_str()))] =  intervalPower.getBpredPower(i) + intervalPower.getClockPower(i) * bpredAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-IntMap").c_str()))] =  intervalPower.getIntRenamePower(i) + intervalPower.getClockPower(i) * intMapAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-FPMap").c_str()))] =  intervalPower.getFpRenamePower(i) + intervalPower.getClockPower(i) * fpMapAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-IntReg").c_str()))] =  intervalPower.getIntRegfilePower(i) + intervalPower.getClockPower(i) * intRegAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-FPReg").c_str()))] =  intervalPower.getFpRegfilePower(i) + intervalPower.getClockPower(i) * fpRegAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-Window").c_str()))] =  intervalPower.getWindowPower(i) + intervalPower.getClockPower(i) * windowAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-IntExec").c_str()))] =  intervalPower.getIntAluPower(i) + intervalPower.getClockPower(i) * windowAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-FPAdd").c_str()))] =  intervalPower.getFpAddPower(i) + intervalPower.getClockPower(i) * fpAddAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-FPMul").c_str()))] =  intervalPower.getFpMulPower(i) + intervalPower.getClockPower(i) * fpMulAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-Dcache").c_str()))] =  intervalPower.getL1dPower(i) + intervalPower.getClockPower(i) * dcacheAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-DTB").c_str()))] =  intervalPower.getDtlbPower(i) + intervalPower.getClockPower(i) * dtbAreaProportion;
    hs_power[get_blk_index(flp, const_cast<char *>((index + "-LdStQ").c_str()))] =  intervalPower.getLsqPower(i) + intervalPower.getClockPower(i) * ldStQAreaProportion;
  }
  for (i = 0; i < numL2Tiles; ++i) {
    ossIndex.str("");
    ossIndex << i;
    index = ossIndex.str();

    hs_power[get_blk_index(flp, const_cast<char *>((index + "-L2").c_str()))] =  intervalPower.getL2uPower(i);
  }

  // find the average power dissipated in the elapsed time
  for (k = 0; k < flp->n_units; ++k) {
    overall_power[k] += hs_power[k];
    // Power array is the sum of the per cycle numbers over the sampling_intvl.
    // Compute the average power per cycle
    hs_power[k] /= cycleInterval;
  }

  // Calculate the current temp given the previous temp, time elapsed since then,
  // and the average power dissipated during that interval
  compute_temp(model, hs_power, hs_temp, model->config->sampling_intvl);

  // Reset the power array for the next interval
  memset(hs_power, 0, flp->n_units * sizeof(double));
}

double TemperatureTracker::getKelvinTemperature(const std::string blockName) {
  if (enabled) {
    return hs_temp[get_blk_index(flp, const_cast<char *>(blockName.c_str()))];
  } else {
    return constantTemp;
  }
}

double TemperatureTracker::getCoreTemperature(const uint32_t i) {
  std::string index;
  std::ostringstream ossIndex;

  ossIndex << i;
  index = ossIndex.str();

  return  std::max(
            std::max(
              std::max(
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-Icache").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-ITB").c_str()))]),
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-Bpred").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-IntMap").c_str()))])
              ),
              std::max(
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-FPMap").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-IntReg").c_str()))]),
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-FPReg").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-Window").c_str()))])
              )
            ),
            std::max(
              std::max(
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-IntExec").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-FPAdd").c_str()))]),
                std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-FPMul").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-Dcache").c_str()))])
              ),
              std::max(hs_temp[get_blk_index(flp, const_cast<char *>((index + "-DTB").c_str()))], hs_temp[get_blk_index(flp, const_cast<char *>((index + "-LdStQ").c_str()))])
            )
          );
}

double TemperatureTracker::getL2Temperature(const uint32_t i) {
  std::ostringstream ossIndex;
  ossIndex << i << "-L2";

  return hs_temp[get_blk_index(flp, const_cast<char *>(ossIndex.str().c_str()))];
}

void TemperatureTracker::reset() {
  if (enabled) {
    memset(overall_power, 0, flp->n_units * sizeof(double));
    cycles = 0;
  }
}
