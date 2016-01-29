/*
  V9 Memory Op
*/

namespace nInorderSimicsFeeder {

class SimicsCycleManager {
  std::vector< std::shared_ptr<SimicsTraceConsumer> > & theConsumers;

public:
  SimicsCycleManager(std::vector< std::shared_ptr<SimicsTraceConsumer> > & aConsumers, bool aClientServer)
    : theConsumers(aConsumers) {
    for (uint32_t i = 0; i < theConsumers.size(); ++i) {
      std::string name("cpu");
      if (aClientServer) {
        name = "server_cpu";
      }
      name += std::to_string(i);
      Simics::API::conf_object_t * cpu = Simics::API::SIM_get_object( name.c_str() );
      DBG_Assert(cpu != 0, ( << "CycleManager cannot locate " << name << " object. No such object in Simics" ));

      Simics::Processor p = Simics::Processor(cpu);
      theConsumers[i]->setInitialCycleCount(p->cycleCount());
    }
  }

  SimicsCycleManager(std::vector< std::shared_ptr<SimicsTraceConsumer> > & aConsumers)
    : theConsumers(aConsumers) {
    for (uint32_t i = 0; i < theConsumers.size(); ++i) {
      Simics::API::conf_object_t * cpu = Simics::API::SIM_get_processor( Simics::ProcessorMapper::mapFlexusIndex2ProcNum(i));
      DBG_Assert(cpu != 0, ( << "CycleManager cannot locate CPU " << i << "(" << Simics::ProcessorMapper::mapFlexusIndex2ProcNum(i) << ") object. No such object in Simics" ));

      Simics::Processor p = Simics::Processor(cpu);
      theConsumers[i]->setInitialCycleCount(p->cycleCount());
    }
  }

  void advanceFlexus() {
    while (allConsumersReady()) {
      theFlexus->doCycle();
    }
  }

  Simics::API::cycles_t reconcileTime(Simics::API::cycles_t aPendingInstructions) {
    Simics::Processor cpu = Simics::Processor::current();
    int32_t simics_id = Simics::APIFwd::SIM_get_processor_number(cpu);
    int32_t current_cpu = Simics::ProcessorMapper::mapProcNum2FlexusIndex(Simics::APIFwd::SIM_get_processor_number(cpu));
    Simics::API::cycles_t simics_cycle_count = cpu->cycleCount() - aPendingInstructions - theConsumers[current_cpu]->initialCycleCount();
    Simics::API::cycles_t flexus_cycle_count = theFlexus->cycleCount();
    //DBG_(Dev, ( << "CPU " << current_cpu << " Reconciling simics: " << simics_cycle_count << " Flexus: " << flexus_cycle_count));
    if (simics_cycle_count < 2 * flexus_cycle_count) {
      return 2 * flexus_cycle_count - simics_cycle_count;
    } else {
      return 0;
    }
    /*        if (simics_cycle_count > flexus_cycle_count) {
                DBG_Assert( (simics_cycle_count < 2 * flexus_cycle_count) );
                return 0;
            } else {
                if (theConsumers.size() == 1) {
                  //For uniprocessors, we return the exact stall time for maximum efficiency
                  return flexus_cycle_count - simics_cycle_count;
                } else {
                  //For muliprocessors, we return 1.  This maximizes our chances of being able to reclaim "slip" cycles
                  return 1;
                }
            }
     */
  }

private:
  bool allConsumersReady() {
    if (theFlexus->quiescing()) {
      return true; //Always ready to advance time when quiescing.
    }
    bool ready = true;
    for (uint32_t i = 0; (i < theConsumers.size()) && ready ; ++i) {
      ready =  (theConsumers[i]->isReady() ||  theConsumers[i]->isInProgress());
    }
    return ready;
  }
};  // struct SimicsCycleManager

}
