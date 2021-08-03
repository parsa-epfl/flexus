#include <components/MemoryLoopback/MemoryLoopbackImpl.cpp>
#include <components/MemoryMap/MemoryMapImpl.cpp>
#include <core/flexus.hpp>
#include <gtest/gtest.h>

namespace Flexus
{
 
	namespace Core
	{	
		void CreateFlexusObject();
		void PrepareFlexusObject();
		void deinitFlexus();
	} // namespace Core  
} // namespace Flexus

// Create fixture for testing the DUT
class MemoryLoopbackTestFixture : public testing::Test
{
	static MemoryMapConfiguration_struct *aMMCfg;		// A dummy configuration
	static MemoryMapJumpTable *aMMJumpTable;		// A dummy jump table
	static nMemoryMap::MemoryMapComponent *aMemoryMap;	// A dummy MemoryMap

protected:
	static void SetUpTestCase()
	{

		// Create Flexus base
		Flexus::Core::PrepareFlexusObject();	
		Flexus::Core::CreateFlexusObject();	

		// Create a memory map to construct a memory map factory object which is used by dut.initialize()
		aMMCfg = new MemoryMapConfiguration_struct("A MemoryMap config");	// A dummy configuration
		aMMJumpTable = new MemoryMapJumpTable;					// A dummy jump table
		Flexus::Core::index_t anIndex = 1;
		Flexus::Core::index_t aWidth =  1;
		aMemoryMap = new nMemoryMap::MemoryMapComponent(			// A dummy MemoryMap instance
			*aMMCfg,
			*aMMJumpTable,
			anIndex,
			aWidth
		);
	}

	static void TearDownTestCase()
	{
		delete aMMCfg;
		delete aMMJumpTable;	
		delete aMemoryMap;
		
		// Exit flexus
		Flexus::Core::deinitFlexus();	
	}	

	void SetUp()
	{
	}	
	void TearDown()
	{
	}
};

MemoryMapConfiguration_struct *MemoryLoopbackTestFixture::aMMCfg = nullptr;		// A dummy configuration
MemoryMapJumpTable *MemoryLoopbackTestFixture::aMMJumpTable = nullptr;			// A dummy jump table
nMemoryMap::MemoryMapComponent *MemoryLoopbackTestFixture::aMemoryMap = nullptr;	// A dummy MemoryMap

#include "delayTest.h"
#include "maxRequestsTest.h"
#include "useFetchReplyTest.h"

//
//bool LoopbackOut_avail(Flexus::Core::index_t)
//{
//	return true;
//}
//
//void LoopbackOut_manip(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
//{
//	if(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type() == Flexus::SharedTypes::MemoryMessage::LoadReply)
//		std::cout << uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address()) << "\n";
//	else
//		std::cout << "Nope\n";
//	std::cout << "Called\n";
//} 
//
//// Create a new test
//TEST(MemoryLoopback, MemoryLoopbackInstantiation)
//{
//	Flexus::Core::PrepareFlexusObject();	
//	Flexus::Core::CreateFlexusObject();	
//
//	// Step 2: Create a configuration struct and specify the parameters
//	MemoryLoopbackConfiguration_struct aCfg("The test config");
//	aCfg.Delay = 2;
//	aCfg.MaxRequests = 3;
//	aCfg.UseFetchReply = false;
//
//	// Step 3: Create a dummy JumpTable and register the ouput port functions
//	MemoryLoopbackJumpTable aJumpTable;
//	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail;	
//	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip;	
//	
//	// Step 4: Specify the Index and Width
//	Flexus::Core::index_t anIndex = 1;
//	Flexus::Core::index_t aWidth =  1;
//
//	// Step 5: Instantiate the DUT
//	nMemoryLoopback::MemoryLoopbackComponent dut(
//	        aCfg,				// Specifies initial config
//		aJumpTable,			// Registers callbacks for output ports
//		anIndex,
//		aWidth
//		);
//
//	// Create a memory map to construct a memory map factory
//	MemoryMapConfiguration_struct aMMCfg("A memory map config");
//	MemoryMapJumpTable aMMJumpTable;
//	nMemoryMap::MemoryMapComponent aMemoryMap(
//		aMMCfg,
//		aMMJumpTable,
//		anIndex,
//		aWidth
//	);
//
//
//	dut.initialize();
//
//	using namespace Flexus;
//	using namespace Core;
//	using namespace SharedTypes;
//
//	using boost::intrusive_ptr;
//
//	MemoryTransport aMemoryTransport;
//	
//	intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(MemoryMessage::LoadReq, PhysicalMemoryAddress(1000)); 
//	aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
//	
//	MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;	
//	dut.push(LoopbackIn_tmp, aMemoryTransport); 
//	theFlexus->doCycle();
//	
//	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
//	dut.drive(LoopbackDrive_tmp);
//	Flexus::Core::deinitFlexus();	
//}
