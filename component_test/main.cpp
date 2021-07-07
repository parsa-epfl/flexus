#include <iostream>
#include <components/MemoryLoopback/MemoryLoopbackImpl.cpp>

int main()
{
	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using Flexus::SharedTypes::MemoryMap;
	using boost::intrusive_ptr;

	MemoryLoopbackConfiguration_struct aCfg("The test config");
	MemoryLoopbackJumpTable aJumpTable;
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	nMemoryLoopback::MemoryLoopbackComponent dut(
	        aCfg,
		aJumpTable,
		anIndex,
		aWidth
		);

	std::cout << "test" << std::endl;
	return -1;
}
