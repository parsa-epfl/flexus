#ifndef FLEXUS_SMMU_STATS
#define FLEXUS_SMMU_STATS

#include <core/stats.hpp>

namespace nSMMU {

namespace Stat = Flexus::Stat;

struct SMMUStats
{
	Stat::StatCounter Total_Translation_Stats;		// Number of tranlsation requests received by SMMU

	Stat::StatCounter Translation_Read_Stats;		// Number of translation requests that were read operations
	Stat::StatCounter Translation_Write_Stats;		// Number of translation requests that were read operations
	
	Stat::StatCounter IOTLB_Hit_Stats;
	Stat::StatCounter IOTLB_Miss_Stats;
	Stat::StatCounter IOTLB_Eviction_Stats;
	Stat::StatCounter IOTLB_Invalidation_Stats;	

	uint64_t Total_Translation;
	uint64_t Translation_Read;
	uint64_t Translation_Write;
	uint64_t IOTLB_Hit;
	uint64_t IOTLB_Miss;
	uint64_t IOTLB_Eviction;
	uint64_t IOTLB_Invalidation;

	SMMUStats(std::string const& theName)
		: Total_Translation_Stats(theName + "-Translation:Total")
		, Translation_Read_Stats(theName + "-Translation:Read")
		, Translation_Write_Stats(theName + "-Translation:Write")
		, IOTLB_Hit_Stats(theName + "-IOTLB:Hit")
		, IOTLB_Miss_Stats(theName + "-IOTLB:Miss")
		, IOTLB_Eviction_Stats(theName + "-IOTLB:Eviction")
		, IOTLB_Invalidation_Stats(theName + "-IOTLB:Invalidation")

	{
		Total_Translation		= 0;
		Translation_Read		= 0;
		Translation_Write		= 0;
		IOTLB_Hit				= 0;
		IOTLB_Miss				= 0;
		IOTLB_Eviction			= 0;
		IOTLB_Invalidation		= 0;
	}

	void update()
	{
		Total_Translation_Stats		+= Total_Translation;
		Translation_Read_Stats		+= Translation_Read;
		Translation_Write_Stats		+= Translation_Write;
		IOTLB_Hit_Stats				+= IOTLB_Hit;
		IOTLB_Miss_Stats			+= IOTLB_Miss;
		IOTLB_Eviction_Stats		+= IOTLB_Eviction;
		IOTLB_Invalidation_Stats	+= IOTLB_Invalidation;

		Total_Translation			= 0;
		Translation_Read			= 0;
		Translation_Write			= 0;
		IOTLB_Hit					= 0;
		IOTLB_Miss					= 0;
		IOTLB_Eviction				= 0;
		IOTLB_Invalidation			= 0;
	}
};

} // namespace nSMMU

#endif /* FLEXUS_SMMU_STATS */
