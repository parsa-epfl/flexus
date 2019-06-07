#include <components/LBAProducer/LBAProducer.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <list>
#include <vector>
#include <core/stats.hpp>
#include <core/flexus.hpp>
//#include <core/simics/simics_interface.hpp>
//#include <core/simics/hap_api.hpp>
#include <core/qemu/mai_api.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <fstream>
#include <boost/tokenizer.hpp>


#include DBG_Control()

/*
namespace Flexus {
namespace Qemu {
namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"
} //namespace API
} //namespace Simics
} //namespace Flexus
*/

#define FLEXUS_BEGIN_COMPONENT LBAProducer
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nLBAProducer {

using namespace Flexus;
using namespace Flexus::SharedTypes;

namespace Qemu = Flexus::Qemu;
namespace API = Qemu::API;

class FLEXUS_COMPONENT(LBAProducer) {
	FLEXUS_COMPONENT_IMPL( LBAProducer );

	public:
	FLEXUS_COMPONENT_CONSTRUCTOR(LBAProducer)
		: base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
	{}

	//InstructionOutputPort
	//=====================
	bool isQuiesced() const {
		return true;
	}

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

	void loadState(std::string const & aDirName) { }

	void initialize(void) {
        SystemWidth = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
				internalValue = 1337;
				theCycleCounter = 0;
				numCores = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
				BaseServiceLatency = cfg.BaseServiceLatency;

				DBG_(Dev, ( << "Allocating waitQueue for " << numCores << "cores."));
				waitQueue = (waitEntry_t*)calloc(numCores, sizeof(waitEntry_t));

				srand48(0xdeadbeef);
				std::string distr = cfg.ServiceTimeDistribution + ":";

                if( cfg.UseServTimeFile == false ) {
                    if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "uniform") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow uniform(" << cfg.DistrArg0 << "), with base service latency = " << BaseServiceLatency << " cycles"));
                        distr += boost::lexical_cast<std::string>(cfg.DistrArg0);
                    } else if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "normal") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow normal(mean="<< cfg.DistrArg0 << ", sd=" << cfg.DistrArg1 << "), with base service latency = " << BaseServiceLatency << " cycles"));
                        distr += boost::lexical_cast<std::string>(cfg.DistrArg0) + "," + boost::lexical_cast<std::string>(cfg.DistrArg1);
                    } else if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "fixed") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow fixed(" << BaseServiceLatency + cfg.DistrArg0 << ")."));
                        distr += boost::lexical_cast<std::string>(cfg.DistrArg0);
                    } else if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "exponential") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow exponential(lambda=" << (1.0/cfg.DistrArg0) << " -> i.e., avg. = " << cfg.DistrArg0 << "), with base service latency = " << BaseServiceLatency << " cycles"));
                        distr += boost::lexical_cast<std::string>(1.0/cfg.DistrArg0);
                    } else if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "pareto") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow GPareto(loc=" << cfg.DistrArg0 << ", scale=" << cfg.DistrArg1
                                                    << ", shape=" << cfg.DistrArg2 << "), with base service latency = " << BaseServiceLatency << " cycles"));
                        distr += boost::lexical_cast<std::string>(cfg.DistrArg0) + "," + boost::lexical_cast<std::string>(cfg.DistrArg1) + "," + boost::lexical_cast<std::string>(cfg.DistrArg2);
                    } else if (strcasecmp(cfg.ServiceTimeDistribution.c_str(), "gev") == 0) {
                        DBG_(Dev, ( << "Service time distribution will follow GEV(loc=" << cfg.DistrArg0 << ", scale=" << cfg.DistrArg1
                                                    << ", shape=" << cfg.DistrArg2 << "), with base service latency = " << BaseServiceLatency << " cycles"));
                        distr += boost::lexical_cast<std::string>(cfg.DistrArg0) + "," + boost::lexical_cast<std::string>(cfg.DistrArg1) + "," + boost::lexical_cast<std::string>(cfg.DistrArg2);
                    } else {
                        DBG_Assert(false, ( << "Unknown ServiceTimeDistribution parameter '" << cfg.ServiceTimeDistribution
                                                                << "'. Accepted options: fixed, normal, exponential, pareto, gev, uniform. See Generator.hpp for details." ));
                    }
                } else {
                    DBG_(Dev, ( << "Service time will be taken from user-provided file: " << cfg.ServTimeFName << ". Number of service times to load: " << cfg.NumElementsInServTimeFile ));
                }

				DBG_(Dev, ( << "Instantiating service time generator " << distr));
				generator = createGenerator(distr,cfg.UseServTimeFile,cfg.ServTimeFName,cfg.NumElementsInServTimeFile);

    }

	//Port
	bool available(interface::MagicInstructionIn const &, index_t anIndex){
		return true;
	}

	void push(interface::MagicInstructionIn const &, index_t anIndex, boost::intrusive_ptr< AbstractInstruction > & anInstruction) {
		DBG_Assert(false);	//currently unused
		/*DBG_Assert(anIndex < SystemWidth);
		boost::intrusive_ptr< Instruction > insn(dynamic_cast< Instruction *>( anInstruction.get() ));

		if(insn->getLBA_LG() != 0)
			get_value_to_lg(anIndex, insn);
		else
			DBG_Assert(false); */
	}

	void registerZeStart(uint64_t theBreakpoint, uint32_t anIndex) {
		DBG_Assert(anIndex < numCores);
		DBG_Assert(!waitQueue[anIndex].targetCycle);
		/*if (waitQueue[anIndex].targetCycle != 0) {
			DBG_(LBA_DEBUG, ( << "Expected 0 counter in core's " << anIndex << " waitQueue, but value is " << waitQueue[anIndex].targetCycle << ". Updating...")); 	//core should be starting new op
			waitQueue[anIndex].targetCycle = theCycleCounter + waitQueue[anIndex].serviceTime;
			break;
		}*/
		uint32_t serviceTime = (uint32_t)floor(generator->generate()) + BaseServiceLatency;
		waitQueue[anIndex].serviceTime = serviceTime;
		waitQueue[anIndex].targetCycle = theCycleCounter + serviceTime;
		DBG_(LBA_DEBUG, ( << "CPU-" << anIndex << " starting processing of a request. Determined service time: " << serviceTime
								<< " (will finish after cycle " << waitQueue[anIndex].targetCycle << ")"));
		uint32_t arg = (uint32_t)theBreakpoint;
		FLEXUS_CHANNEL_ARRAY(CoreControlFlow, anIndex) << arg;
	}

	//void get_value_to_lg(index_t anIndex,  boost::intrusive_ptr< Instruction > anInsn){
	void control_the_flow(uint64_t theBreakpoint, uint32_t anIndex) {
		//switch(anInsn->getLBA_LG()){
		switch(theBreakpoint){
			case 101: {
				registerZeStart(theBreakpoint, anIndex);
				break;
			}
			case 102:
				if (!waitQueue[anIndex].targetCycle) {
					DBG_(LBA_DEBUG, ( << "Ooops! Received 102 breakpoint, which should have been preceded by a 101 breakpoint..."));
					registerZeStart(101, anIndex);
					break;
				}
				if (waitQueue[anIndex].targetCycle <= theCycleCounter) {	//time to let core finish this "request"
					//anInsn->setValueToLG(internalValue);
					API::conf_object_t * theCPU = API::QEMU_get_cpu_by_index(anIndex);

                    // Msutherl - reads the x0 register for QEMU which is where the first value of the breakpoint is stored
					API::QEMU_write_register( theCPU , API::arm_register_t::kGENERAL, 0 , internalValue );
                    // End QEMU port change

					DBG_(LBA_DEBUG, ( << "CPU-" << anIndex << " finished processing a request. Was expecting this to finish " << theCycleCounter - waitQueue[anIndex].targetCycle << " cycles ago. "));
					DBG_(Iface, ( << "Writing the value " << internalValue << " to register l0"));
					waitQueue[anIndex].targetCycle = 0;	//reset
					uint32_t arg = (uint32_t)theBreakpoint;
					FLEXUS_CHANNEL_ARRAY(CoreControlFlow, anIndex) << arg;
				} else {
					//DBG_(LBA_DEBUG, ( << "Core " << anIndex << " spinning until request completion (determined finish time: " << waitQueue[anIndex] << ")"));
				}
				break;
			default:
				//DBG_Assert(false, ( << "Unknown ID for requesting values from the simulator - " << anInsn->getLBA_LG()));
				DBG_Assert(false, ( << "Breakpoint with unknown argument: " << theBreakpoint));
		}
	}

	// Port
  FLEXUS_PORT_ALWAYS_AVAILABLE(FunctionIn);
  void push(interface::FunctionIn const &, RMCEntry & anEntry) {
        if (cfg.EnableServiceTimeGen) {
		    control_the_flow(anEntry.getSize(), (uint32_t)anEntry.getCPUid());
	} else {
			uint32_t arg = (uint32_t)anEntry.getSize();
		    FLEXUS_CHANNEL_ARRAY(CoreControlFlow,(uint32_t)anEntry.getCPUid()) << arg;
        }
            
	/*	uint32_t cpu_id = anEntry.getCPUid();

		uint32_t random = rand()%1000;
		DBG_(LBA_DEBUG, ( << "Received breakpoint from core " << cpu_id << ". Will write the value " << random << " to register l0"));
		API::conf_object_t *theCPU = API::SIM_get_processor(cpu_id);
		Simics::API::SIM_write_register( theCPU , Simics::API::SIM_get_register_number ( theCPU , "l0") , random );*/
	}

	void drive(interface::Cycle const &) {
		theCycleCounter++;
	}

	void finalize () { }

	private:

	uint32_t SystemWidth, numCores, BaseServiceLatency;
	uint64_t internalValue, theCycleCounter;
	Generator *generator;
	typedef struct waitEntry {
		uint64_t serviceTime, targetCycle;
	} waitEntry_t;
	waitEntry_t *waitQueue;

};  // end class LBAProducer

}  // end Namespace LBAProducer

FLEXUS_COMPONENT_INSTANTIATOR( LBAProducer, nLBAProducer);

FLEXUS_PORT_ARRAY_WIDTH ( LBAProducer, MagicInstructionIn ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH ( LBAProducer, CoreControlFlow ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT LBAProducer

#define DBG_Reset
#include DBG_Control()
