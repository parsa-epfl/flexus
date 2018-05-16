// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_uARCH_VALUETRACKER_HPP_INCLUDED
#define FLEXUS_uARCH_VALUETRACKER_HPP_INCLUDED

#include <map>
#include <iostream>

#include <core/performance/profile.hpp>
#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>

#include <core/qemu/configuration_api.hpp>

//ALEX - Remove the following if running w/ Qemu instead of Simics
#ifndef CONFIG_QEMU
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
} //namespace Qemu
} //namespace Flexus
#else
#include <core/qemu/api.h>
#endif

namespace API = Flexus::Qemu::API;

#include <components/CommonQEMU/Slices/MemOp.hpp>

#define DBG_DeclareCategories Special
#include DBG_Control()

namespace nuArchARM {
using namespace Flexus::SharedTypes;

inline uint64_t mask( eSize aSize) {
  switch (aSize) {
    case kByte:
      return 0xFFULL;
    case kHalfWord:
      return 0xFFFFULL;
    case kWord:
      return 0xFFFFFFFFULL;
    case kDoubleWord:
      return 0xFFFFFFFFFFFFFFFFULL;
  }
  return 0xFFFFFFFFFFFFFFFFULL;
}

struct LocalValue {
  uint64_t theValue;
  uint32_t theOutstandingStores;
  LocalValue(uint64_t aValue)
    : theValue(aValue)
    , theOutstandingStores(1)
  { }

};

typedef enum {
  kSimicsReflectsG = -1,
  kPoisonedByDMA = -2
} eValueTrackSpecialValues;

struct ValueTrack {
  int32_t theSimicsReflectsCPU;
  uint64_t theGloballyVisibleValue;
  typedef std::map< uint32_t, LocalValue> local_values;
  local_values theLocallyVisibleValues;

  ValueTrack( int32_t aCPU, uint64_t aLocalValue, uint64_t aGlobalValue) {
    theSimicsReflectsCPU = aCPU;
    theGloballyVisibleValue = aGlobalValue;
    theLocallyVisibleValues.insert (std::make_pair( aCPU, LocalValue(aLocalValue) ) );
  }

  friend std::ostream & operator << ( std::ostream & anOstream, ValueTrack const & t) {
    anOstream << " G:" << std::hex << t.theGloballyVisibleValue << std::dec ;
    local_values::const_iterator iter = t.theLocallyVisibleValues.begin();
    while (iter != t.theLocallyVisibleValues.end()) {
      anOstream << " " << boost::padded_string_cast < 2, '0' > (iter->first) << ":" << std::hex << iter->second.theValue << std::dec << "<" << iter->second.theOutstandingStores << ">";
      ++iter;
    }
    if (t.theSimicsReflectsCPU == kSimicsReflectsG) {
      anOstream << ". Simics Reflects: G";
    } else if (t.theSimicsReflectsCPU == kPoisonedByDMA) {
      anOstream << ". Simics poisoned by DMA";
    } else {
      anOstream << ". Simics Reflects: " << t.theSimicsReflectsCPU;
    }
    return anOstream;
  }
};

class DMATracerImpl {
  API::conf_object_t * theUnderlyingObject;
  API::conf_object_t * theMapObject;
  int theVM;

public:
  DMATracerImpl(API::conf_object_t * anUnderlyingObjec)
    : theUnderlyingObject(anUnderlyingObjec)
  {}

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t * aMapObject, int aVM) {
    theMapObject = aMapObject;
    theVM = aVM;
//ALEX - Not setting up tracer for now. 
//
    //API::attr_value_t attr;
    //attr.kind = API::Sim_Val_Object;
    //attr.u.object = theUnderlyingObject;

    /* Tell memory we have a mem hier */	
    //API::SIM_set_attribute(theMapObject, "timing_model", &attr);
  }

  //ALEX - WARNING: Disabled dma for now. Also in ValueTracker.cpp
  //API::cycles_t dma_mem_hier_operate(API::conf_object_t * space, API::map_list_t * map, API::generic_transaction_t * aMemTrans);

};  // class DMATracerImpl

class DMATracer : public Flexus::Qemu::AddInObject <DMATracerImpl> {
  typedef Flexus::Qemu::AddInObject<DMATracerImpl> base;
public:
  static const Flexus::Qemu::Persistence  class_persistence = Flexus::Qemu::Session;
  static std::string className() {
    return "DMATracer";
  }
  static std::string classDescription() {
    return "Flexus's DMA tracer.";
  }

  DMATracer() : base() { }
  DMATracer(API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  DMATracer(DMATracerImpl * anImpl) : base(anImpl) {}
};

struct ValueTracker {
  static ValueTracker ** theGlobalTracker;
  static int theNumTrackers;
  static ValueTracker & valueTracker(int vm) {
    if (!theGlobalTracker) {
      theNumTrackers = Flexus::Qemu::ProcessorMapper::numVMs();
      if (theNumTrackers == 0){
        theNumTrackers = 1;
      }
      theGlobalTracker = new ValueTracker*[theNumTrackers];
      for (int i=0;i<theNumTrackers;i++){
        theGlobalTracker[i] = new ValueTracker;
        theGlobalTracker[i]->register_mem_iface(i);
      }
    }
    return *theGlobalTracker[vm];
  }

  typedef std::map< PhysicalMemoryAddress, ValueTrack > tracker;
  tracker theTracker;
  DMATracer theDMATracer;

  void register_mem_iface(int vm) {
    DBG_(Tmp, ( << "Registering DMA tracker " << vm));

    API::conf_object_t * dma_map_object = API::QEMU_get_object( "dma_mem" );
    API::QEMU_clear_exception();
    if (! dma_map_object) {
      std::string dma_map_name = "dma_mem" + std::to_string(vm);
      dma_map_object=API::QEMU_get_object( dma_map_name.c_str());
    }

   DBG_(Crit, ( << "ALEX -- WARNING: DMA tracker has not been set up (Needs to be fixed)"));
//ALEX - Need to handle dma! Temporarily commented out the following:
/*
    if (! dma_map_object) {
      bool client_server = false;
      DBG_( Dev, ( << "Creating DMA map object" ) );
      std::string cpu_name = "machine" + std::to_string(vm) + "_cpu0";
      std::string cpu_mem_name = cpu_name + "_mem";
      API::conf_object_t * cpu0_mem = API::QEMU_get_object(cpu_mem_name.c_str());
      API::conf_object_t * cpu0 = API::QEMU_get_object(cpu_name.c_str());
      if ((vm == 0) && (!cpu0_mem)){
          cpu0_mem = API::QEMU_get_object("cpu0_mem");
          cpu0 = API::QEMU_get_object("cpu0");
      }

      if ( ! cpu0_mem ) {
        client_server = true;
	cpu_name = "machine" + std::to_string(vm) + "_server_cpu0";
	//cpu_mem_name = "machine" + std::to_string(vm) + "_server_server_cpu0_mem";
        cpu_mem_name = "server_machine" + std::to_string(vm) + "_server_cpu0_mem";
	cpu0_mem = API::QEMU_get_object(cpu_mem_name.c_str());
	cpu0 = API::QEMU_get_object(cpu_name.c_str());
	if ((vm == 0) && (!cpu0_mem)){
	  cpu0_mem = API::QEMU_get_object("server_cpu0_mem");
	  cpu0 = API::QEMU_get_object("server_cpu0");
        }
        DBG_Assert(cpu0_mem, ( << "Unable to connect DMA because there is no cpu0_mem"));
      }
      
      API::attr_value_t map_key = API::SIM_make_attr_string("map");
      API::attr_value_t map_value = API::SIM_get_attribute(cpu0_mem, "map");
      API::attr_value_t map_pair = API::SIM_make_attr_list(2, map_key, map_value);
      API::attr_value_t map_list = API::SIM_make_attr_list(1, map_pair);
      API::conf_class_t * memory_space = API::SIM_get_class( "memory-space" );
      std::string dma_map_name = "dma_mem" + std::to_string(vm);
      dma_map_object = SIM_create_object(memory_space, dma_map_name.c_str(), map_list);
      DBG_Assert( dma_map_object, ( << "Failed to create object ' " << dma_map_name << "'"));
      API::conf_class_t * schizo = API::SIM_get_class( "serengeti-schizo" );
      API::attr_value_t dma_attr;
      dma_attr.kind = API::Sim_Val_Object;
      dma_attr.u.object = dma_map_object;

      API::attr_value_t all_objects = API::SIM_get_all_objects();
      DBG_Assert(all_objects.kind == API::Sim_Val_List);
      for (int32_t i = 0; i < all_objects.u.list.size; ++i) {
        if (all_objects.u.list.vector[i].u.object->class_data == schizo && API::SIM_get_attribute( all_objects.u.list.vector[i].u.object, "queue").u.object == cpu0 ) {
          API::SIM_set_attribute(all_objects.u.list.vector[i].u.object, "memory_space", &dma_attr );
        }
      }
      
    }

    //Create SimicsTracer Factory
    Flexus::Qemu::Factory<DMATracer> tracer_factory;
    API::conf_class_t * trace_class = tracer_factory.getSimicsClass();

    API::timing_model_interface_t * timing_interface;
    timing_interface = new API::timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = &Flexus::Qemu::make_signature_from_addin_fn2<API::operate_func_t>::with<DMATracer, DMATracerImpl, &DMATracerImpl::dma_mem_hier_operate>::trampoline;
    API::SIM_register_interface(trace_class, "timing-model", timing_interface);

    std::string tracer_name("dma-tracer");
    tracer_name += std::to_string(vm);
    theDMATracer = tracer_factory.create(tracer_name);
    DBG_( Crit, ( << "Connecting to DMA memory map" ) );
    theDMATracer->init(dma_map_object,vm);

    DBG_(Tmp, ( << "Done registering DMA tracker"));
*/
  }

  void access( uint32_t aCPU, PhysicalMemoryAddress anAddress ) {
    FLEXUS_PROFILE();
    DBG_( Iface, ( << "CPU[" << aCPU << "] Access " << anAddress ) );
    DBG_Assert( anAddress < 0x40000000000LL );

    //Align the address
    PhysicalMemoryAddress aligned = dwAddr( anAddress );

    //See if we already have a ValueTrack
    tracker::iterator iter = theTracker.find( aligned );
    if ( iter == theTracker.end() ) {
      DBG_( Iface, ( << "CPU[" << aCPU << "] Access.NothingOutstanding " << anAddress ) );
      return;
    } else {
      if (iter->second.theSimicsReflectsCPU == kPoisonedByDMA) {
        // the globally visible value was updated by DMA, so update it just in case we use it later
        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
        iter->second.theGloballyVisibleValue = cpu->readPAddr(aligned, 8);
        iter->second.theSimicsReflectsCPU = kSimicsReflectsG;
        DBG_( Iface, AddCategory(Special) ( << "CPU[" << aCPU << "] Access.UpdatedGlobalWithDMA " << anAddress << " Now: "  << iter->second ) );
      }
      ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find( aCPU );
      if (local != iter->second.theLocallyVisibleValues.end() ) {
        if (iter->second.theSimicsReflectsCPU == static_cast<int>(aCPU)) {
          //Simics currently has the value for this CPU.  We update the tracker
          //with the new value
          Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
          uint64_t simics_value = cpu->readPAddr( aligned, 8 );

          if (simics_value != local->second.theValue) {
            DBG_( Trace, ( << "CPU[" << aCPU << "] Access.SimicsMoreRecent " << anAddress << " " << std::hex << simics_value << " Expected: " << local->second.theValue << std::dec ) );
            //DBG_Assert( false );
            local->second.theValue = simics_value;
          }
          //When the store is completed, Simics will reflect the value visible to
          //this processor

          DBG_( Iface, ( << "CPU[" << aCPU << "] Access.SimicsReflectsCorrectCPU " << anAddress << " Now: "  << iter->second ) );

        } else {
          //Simics currently has the value for some other CPU.  We overwrite
          //Simics memory with the correct value for this CPU.

          Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

          uint64_t current_value = local->second.theValue;

          //Change simics' value to reflect what this CPU believes memory
          //looks like
          cpu->writePAddr( aligned, 8, current_value );
          iter->second.theSimicsReflectsCPU = aCPU;

          DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Access.SimicsSwitchedToCorrectCPU " << anAddress << " Now: "  << iter->second ) );
        }

      } else {
        //No previous value for this CPU.  Start with the globally visible
        //value

        if ( iter->second.theSimicsReflectsCPU == kSimicsReflectsG) {
          DBG_( Iface, ( << "CPU[" << aCPU << "] Access.SimicsReflectsCorrectGlobal " << anAddress << " Now: "  << iter->second ) );
        } else {
          Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

          uint64_t current_value = iter->second.theGloballyVisibleValue;
          //Change simics' value to reflect what this CPU believes memory
          //looks like
          cpu->writePAddr( aligned, 8, current_value );
          iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

          DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Access.SwitchedToGlobalValue " << anAddress << " Now: "  << iter->second ) );
        }
      }
    }
  }

  void store( uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize, uint64_t aStoreValue) {
    FLEXUS_PROFILE();
    DBG_Assert( anAddress != 0 );
    DBG_Assert( anAddress < 0x40000000000LL );
    DBG_( Iface, ( << "CPU[" << aCPU << "] Store " << anAddress << "[" << aSize << "] = " << std::hex << aStoreValue << std::dec ) );

    //Align the address
    PhysicalMemoryAddress aligned = dwAddr( anAddress );

    //See if we already have a ValueTrack
    tracker::iterator iter = theTracker.find( aligned );
    if ( iter == theTracker.end() ) {
      Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
      uint64_t simics_value = cpu->readPAddr( aligned, 8 );
      //New tracker
      uint64_t updated_value = overlay( simics_value, anAddress, aSize, aStoreValue);

      tracker::iterator iter;
      bool ignored;
      std::tie(iter, ignored) = theTracker.insert( std::make_pair ( aligned, ValueTrack( aCPU, updated_value, simics_value) ) );
      //When the store is completed, Simics will reflect the value visible to
      //this processor
      cpu->writePAddr( aligned, 8, updated_value );

      DBG_( Iface, ( << "CPU[" << aCPU << "] Store.New " << anAddress << " no prior outstanding values. Now: "  << iter->second ) );

    } else {
      if (iter->second.theSimicsReflectsCPU == kPoisonedByDMA) {
        // the globally visible value was updated by DMA, so update it just in case we use it later
        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
        iter->second.theGloballyVisibleValue = cpu->readPAddr(aligned, 8);
        iter->second.theSimicsReflectsCPU = kSimicsReflectsG;
        DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Store.UpdatedGlobalWithDMA " << anAddress << " Now: "  << iter->second ) );
      }

      ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find( aCPU );
      if (local != iter->second.theLocallyVisibleValues.end() ) {
        if (iter->second.theSimicsReflectsCPU == static_cast<int>(aCPU)) {
          //Simics currently has the value for this CPU.  We update the tracker
          //with the new value
          Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
          uint64_t simics_value = cpu->readPAddr( aligned, 8 );

          if (simics_value != local->second.theValue) {
            DBG_( Trace, ( << "Simics value more recent than ValueTracker: " << std::hex << simics_value << " Expected: " << local->second.theValue << std::dec ) );
            //DBG_Assert(false);
          }
          uint64_t updated_value = overlay( simics_value, anAddress, aSize, aStoreValue);
          local->second.theValue = updated_value;
          local->second.theOutstandingStores++;
          //When the store is completed, Simics will reflect the value visible to
          //this processor
          cpu->writePAddr ( aligned, 8, updated_value );

          DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Store.UpdateByCurrent " << anAddress << " Now: "  << iter->second ) );

        } else {
          //Simics currently has the value for some other CPU.  We overwrite
          //Simics memory with the correct value for this CPU.

          Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

          uint64_t current_value = local->second.theValue;
          uint64_t updated_value = overlay( current_value, anAddress, aSize, aStoreValue);

          //Change simics' value to reflect what this CPU believes memory
          //looks like
          cpu->writePAddr( aligned, 8, updated_value );
          iter->second.theSimicsReflectsCPU = aCPU;

          local->second.theValue = updated_value;
          local->second.theOutstandingStores++;

          DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Store.UpdateByExisting " << anAddress << " Now: "  << iter->second ) );

        }
      } else {
        //No previous value for this CPU.  Start with the globally visible
        //value

        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

        uint64_t current_value = iter->second.theGloballyVisibleValue;
        uint64_t updated_value = overlay( current_value, anAddress, aSize, aStoreValue);

        //Change simics' value to reflect what this CPU believes memory
        //looks like
        cpu->writePAddr( aligned, 8, updated_value );
        iter->second.theSimicsReflectsCPU = aCPU;

        iter->second.theLocallyVisibleValues.insert( std::make_pair( aCPU, LocalValue( updated_value ) ) );

        DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Store.UpdateByNew " << anAddress << " Now: "  << iter->second ) );
      }
    }
  }

  void commitStore( uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize, uint64_t aStoreValue) {
    FLEXUS_PROFILE();
    DBG_Assert( anAddress != 0 );
    DBG_Assert( anAddress < 0x40000000000LL );
    DBG_( Iface, ( << "CPU[" << aCPU << "] CommitStore " << anAddress << "[" << aSize << "] = " << std::hex << aStoreValue << std::dec ) );

    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

    //Align the address
    PhysicalMemoryAddress aligned = dwAddr( anAddress );

    //We must have a ValueTrack
    tracker::iterator iter = theTracker.find( aligned );
    DBG_Assert ( iter != theTracker.end() );
    ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find( aCPU );
    DBG_Assert ( local != iter->second.theLocallyVisibleValues.end() );

    if (iter->second.theSimicsReflectsCPU == kPoisonedByDMA) {
      // the globally visible value was updated by DMA, so update it just in case we use it later
      iter->second.theGloballyVisibleValue = cpu->readPAddr(aligned, 8);
      iter->second.theSimicsReflectsCPU = kSimicsReflectsG;
      DBG_( Iface, AddCategory(Special) ( << "CPU[" << aCPU << "] CommitStore.UpdatedGlobalWithDMA " << anAddress << " Now: "  << iter->second ) );
    }

    //Compute the new globally visible value
    uint64_t current_value = iter->second.theGloballyVisibleValue;
    uint64_t updated_value = overlay( current_value, anAddress, aSize, aStoreValue);
    iter->second.theGloballyVisibleValue = updated_value;

    /*
    // To debug store problems (e.g., where the store value matches with Simics but the actual
    // bytes written do not), set Flexus to sequential consistency, disable writePAddr() in
    // store() above, and enable the check below. Be careful about data races, which can
    // appear here as mismatches, even under SC.
    uint64_t simics_value = cpu->readPAddr( aligned, 8);
    if(simics_value != updated_value) {
      DBG_(Dev, ( << "CPU[" << aCPU << "] memory mismatch: Simics: " << std::hex << simics_value << "  Flexus: " << updated_value ) );
    }
    */

    //Change simics to reflect the globally visible value
    cpu->writePAddr( aligned, 8, updated_value );
    iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

    //Decrement outstanding store count
    local->second.theOutstandingStores--;
    if (local->second.theOutstandingStores == 0) {
      DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] CommitStore.LastOutstandingStoreForCPU " << anAddress ) );
      iter->second.theLocallyVisibleValues.erase(local);
    }
    if (iter->second.theLocallyVisibleValues.empty() ) {
      DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] CommitStore.LastOutstandingStore " << anAddress ) );
      theTracker.erase(iter);
    } else {
      DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] CommitStore.ResultingState " << anAddress << " Now: " << iter->second) );
    }
  }

  uint64_t load( uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize) {
    FLEXUS_PROFILE();
    DBG_Assert( anAddress != 0 );
    DBG_Assert( aSize <= 8 );
    DBG_Assert( anAddress < 0x40000000000LL );
    DBG_( Iface, ( << "CPU[" << aCPU << "] Load " << anAddress << "[" << aSize << "]" ) );

    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

    //Align the address
    PhysicalMemoryAddress aligned = dwAddr( anAddress );

    //See if we already have a ValueTrack
    tracker::iterator iter = theTracker.find( aligned );
    if ( iter == theTracker.end() ) {
      uint64_t val = cpu->readPAddr( anAddress, aSize );
      DBG_( Iface, ( << "CPU[" << aCPU << "] Load.NoOutstandingValues " << anAddress << "[" << aSize << "] = " << std::hex << val << std::dec ) );
      return val;
    }

    if (iter->second.theSimicsReflectsCPU == kPoisonedByDMA) {
      // the globally visible value was updated by DMA, so update it just in case we use it later
      iter->second.theGloballyVisibleValue = cpu->readPAddr(aligned, 8);
      iter->second.theSimicsReflectsCPU = kSimicsReflectsG;
      DBG_( Iface, AddCategory(Special) ( << "CPU[" << aCPU << "] Load.UpdatedGlobalWithDMA " << anAddress << " Now: "  << iter->second ) );
    }

    ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find( aCPU );
    if (local == iter->second.theLocallyVisibleValues.end() ) {
      if ( iter->second.theSimicsReflectsCPU != kSimicsReflectsG) {
        //Change simics to reflect the globally visible value
        cpu->writePAddr( aligned, 8, iter->second.theGloballyVisibleValue );
        iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

        uint64_t val = cpu->readPAddr( anAddress, aSize );
        DBG_( Trace, AddCategory(Special)( << "CPU[" << aCPU << "] Load.SwitchToGlobalValue " << anAddress << "[" << aSize << "] = " << std::hex << val << std::dec ) );
        return val;
      } else {
        uint64_t val = cpu->readPAddr( anAddress, aSize );
        DBG_( Iface, ( << "CPU[" << aCPU << "] Load.SimicsAlreadySetToGlobal " << anAddress << "[" << aSize << "] = " << std::hex << val << std::dec ) );
        return val;
      }
    }

    if ( iter->second.theSimicsReflectsCPU != static_cast<int>(aCPU)) {

      //Change simics to reflect the value for this CPU
      cpu->writePAddr( aligned, 8, local->second.theValue );
      iter->second.theSimicsReflectsCPU = aCPU;

      uint64_t val = cpu->readPAddr( anAddress, aSize );
      DBG_( Trace, AddCategory(Special) ( << "CPU[" << aCPU << "] Load.SwitchToLocalValue " << anAddress << "[" << aSize << "] = " << std::hex << val << std::dec ) );
      return val;
    } else {

      uint64_t val = cpu->readPAddr( anAddress, aSize );
      DBG_( Iface, ( << "CPU[" << aCPU << "] Load.SimicsAlreadySetToLocal " << anAddress << "[" << aSize << "] = " << std::hex << val << std::dec ) );
      return val;
    }
  }

  void invalidate(PhysicalMemoryAddress anAddress, eSize aSize) {
    DBG_(Iface, ( << "VT invalidating addr:" << anAddress << " size:" << aSize));
    PhysicalMemoryAddress addr = anAddress;

    for (int32_t i = 0; i < aSize; i++) {
      PhysicalMemoryAddress aligned = dwAddr(addr);
      tracker::iterator iter = theTracker.find( aligned );

      if ( iter != theTracker.end() ) {
        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(0);
        DBG_(Iface, ( << "VT invalidating entry due to DMA on " << aligned << " entry: " << iter->second << " simics_value=" <<  cpu->readPAddr( aligned, 8 )));
        iter->second.theGloballyVisibleValue = 0xbaadf00d;
        iter->second.theSimicsReflectsCPU = kPoisonedByDMA;
      }
      addr += 1;
    }
  }

private:
  //Value Manipulation
  //==========================================================================
  PhysicalMemoryAddress dwAddr(PhysicalMemoryAddress anAddress) {
    return PhysicalMemoryAddress( anAddress & ~7 );
  }

  int32_t offset (PhysicalMemoryAddress anAddress, eSize aSize) {
    return ( 8 - aSize ) - (static_cast<uint64_t>( anAddress ) - dwAddr(anAddress) );
  }

  uint64_t makeMask(PhysicalMemoryAddress anAddress, eSize aSize) {
    uint64_t mask = nuArchARM::mask(aSize);
    mask <<= ( offset(anAddress, aSize) * 8 );
    return mask;
  }

  uint64_t align( uint64_t aValue, PhysicalMemoryAddress anAddress, eSize aSize) {
    return ( aValue << ( offset(anAddress, aSize) * 8) );
  }

  uint64_t overlay( uint64_t anOriginalValue, PhysicalMemoryAddress anAddress, eSize aSize, uint64_t aValue) {
    uint64_t available = makeMask(anAddress, aSize) ;           //Determine what part of the int64_t the update covers
    uint64_t update_aligned = align(aValue, anAddress, aSize);  //align the update value for computation
    anOriginalValue &= ( ~available );                                    //zero out the overlap region in the original value
    anOriginalValue |= ( update_aligned & available );                    //paste the update value into the overlap
    return anOriginalValue;
  }
};

} //nuArchARM

#endif //FLEXUS_uARCH_VALUETRACKER_HPP_INCLUDED
