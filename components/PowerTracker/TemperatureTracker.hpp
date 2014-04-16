#ifndef FLEXUS_TEMPERATURE_TRACKER_HPP_INCLUDED
#define FLEXUS_TEMPERATURE_TRACKER_HPP_INCLUDED

// The TemperatureTracker provides an interface to HotSpot, which is available
// at http://lava.cs.virginia.edu/HotSpot. The HotSpot license is available in
// components/PowerTracker/HotSpot/LICENSE.

#include <components/PowerTracker/HotSpot/temperature.h>
#include <components/PowerTracker/PerBlockPower.hpp>
#include <string>

class TemperatureTracker {
public:
  TemperatureTracker() : enabled(false) {}
  ~TemperatureTracker();

  void initialize(thermal_config_t & config, const std::string floorplanFile, const uint32_t interval, const uint32_t aNumCoreTiles, const uint32_t aNumL2Tiles);

  void addToCycles(const uint32_t x) {
    cycles += x;
  }

  void updateTemperature(const PerBlockPower & intervalPower);
  double getKelvinTemperature(const std::string blockName);
  double getCoreTemperature(const uint32_t i);
  double getL2Temperature(const uint32_t i);
  void setConstantTemperature(const double temp) {
    constantTemp = temp + 273.15;
  }
  void writeSteadyStateTemperatures();

  void reset();

private:
  bool enabled;
  double constantTemp;

  flp_t * flp;        // floorplan
  RC_model_t * model;      // temperature model
  double * hs_temp, *hs_power;    // instantaneous temperature and power values
  double * overall_power, *steady_temp; // steady state temperature and power values
  double * avg_power;
  uint32_t cycles;

  uint32_t numCoreTiles;
  uint32_t numL2Tiles;
  uint32_t cycleInterval;

  // Block area as a proportion of total area. Used to assign clock power
  double icacheAreaProportion,
         dcacheAreaProportion,
         bpredAreaProportion,
         dtbAreaProportion,
         fpAddAreaProportion,
         fpRegAreaProportion,
         fpMulAreaProportion,
         fpMapAreaProportion,
         intMapAreaProportion,
         windowAreaProportion,
         intRegAreaProportion,
         intExecAreaProportion,
         ldStQAreaProportion,
         itbAreaProportion;
};
#endif
