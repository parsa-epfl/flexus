double calculateL2AccessCost(const double frq, const double vdd, const int32_t numMemFus, const int32_t l2uNumSets, const int32_t l2uBlockSize, const int32_t l2uAssociativity, const int32_t virtualAddressSize);
double calculateNiBufferAccessCost(const double frq, const double vdd, const int32_t numEntries, const int32_t entrySize);
double perL2ClockPower(double tile_width, double tile_height, double die_width, double die_height, const double frq, const double vdd, uint32_t systemWidth);
