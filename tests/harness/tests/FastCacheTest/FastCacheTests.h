#include <gtest/gtest.h>

#include <components/CommonQEMU/TraceTracker.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/RegionScoutMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#include <components/FastCache/LookupResult.hpp>
#include <components/FastCache/CoherenceProtocol.hpp>
#include <components/FastCache/InclusiveMOESI.hpp>
#include <components/FastCache/AbstractCache.hpp>
#include <components/FastCache/CacheStats.hpp>
#include <components/FastCache/RTCache.hpp>
#include <components/FastCache/StdCache.hpp>

#include <core/performance/profile.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
// Test fixture for the FastCache component

TEST_F(FastCacheTestFixture, FastCacheTest) {

    FastCacheConfiguration_struct aCfg("FastCacheTest config");
	InitializeFastCacheConfiguration(aCfg,1, 65536, 2, 64, false, Flexus::SharedTypes::eUnknown, false, false, false, 1024, 16, 8192, "SetLRU" , 8, false, false, false, "InclusiveMESI" , true, false, true, false, false);

    FastCacheJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "FastCache index and widths defined\n";
	// Step 5: Instantiate the DUT
	nFastCache::FastCacheComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "FastCacheComponent instantiated\n";
	
	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nFastCache;

    // Initialize the component
    dut.initialize();

	std::cout << "FastCacheComponent initialized\n";

// Input Initializations
/*
 // Declare and initialize input
    boost::intrusive_ptr<BranchFeedback> dummyFeedback(new BranchFeedback());
// Pushes to the FastCache
    dut.push(FastCacheInterface::BranchFeedbackIn(), 0, dummyFeedback);
    std::cout<<"dut push done\n";

//Drive The FastCache
	FastCacheInterface::FastCacheDrive drive_temp;
	dut.drive(drive_temp);

*/
   
// TESTS: Perform assertions based on the expected behavior

    // ASSERT_EQ(expectedTraceInModernResult, pcAndTypeAndAnnulPair);

    // Finalize the component
    dut.finalize();
}
