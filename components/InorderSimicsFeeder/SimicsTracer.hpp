/*
  V9 Memory Op
*/

namespace nInorderSimicsFeeder {

std::set<Flexus::Simics::API::conf_object_t *> theTimingModels;

class SimicsTracerImpl  {
private:
  Simics::API::conf_object_t * theUnderlyingObject;
  Simics::API::conf_object_t * theCPU;
  index_t theIndex;
  std::shared_ptr<SimicsTraceConsumer> theConsumer;
  SimicsCycleManager * theCycleManager; //Non-owning pointer
  StoreBuffer theStoreBuffer;
  bool theInterruptsEnabled;
  bool theIgnoreFlag;
  Simics::API::conf_object_t * thePhysMemory;
  Simics::API::conf_object_t * thePhysIO;
  Simics::API::physical_address_t current_pc;
  int64_t theStallCap;

  int32_t x86_interrupt_problem;
  uint64_t x86_interrupt_problem_counter;

public:
  //Default Constructor, creates a SimicsTracer that is not connected
  //to an underlying Simics object.
  SimicsTracerImpl(Simics::API::conf_object_t * anUnderlyingObject)
    : theUnderlyingObject(anUnderlyingObject)
    , theConsumer()
    , theInterruptsEnabled(true)
    , theIgnoreFlag(false)
    , theStallCap(5000) {
    x86_interrupt_problem = 0;
    x86_interrupt_problem_counter = 0;
  }

  // Initialize the tracer to the desired CPU
  void init(Simics::API::conf_object_t * aCPU, index_t anIndex, int64_t aStallCap) {
    theCPU = aCPU;
    theIndex = anIndex;
    theStallCap = aStallCap;

    Simics::API::attr_value_t attr;

    theIndex = anIndex;

    if (Simics::API::SIM_class_has_attribute(theCPU->class_data, "turbo_execution_mode")) {
      DBG_Assert( false , ( << "Simics appears to be running with the -fast option.  You must launch Simics with -stall to run in-order simulations") );
    }

    attr = Simics::API::SIM_get_attribute(theCPU, "ooo_mode");
    if ( attr.kind != Simics::API::Sim_Val_String || std::string(attr.u.string) != "in-order" ) {
      DBG_Assert( false , ( << "Simics appears to be running with the -ma option.  You must launch Simics with -stall to run in-order simulations") );
    }

    attr.kind = Simics::API::Sim_Val_Object;
    attr.u.object = theCPU;

    Simics::API::SIM_set_attribute(theUnderlyingObject, "queue", &attr);

    attr.kind = Simics::API::Sim_Val_Object;
    attr.u.object = theUnderlyingObject;

    /* Tell memory we have a mem hier */
    thePhysMemory = Simics::API::SIM_get_attribute(theCPU, "physical_memory").u.object;
    Simics::API::SIM_set_attribute(thePhysMemory, "timing_model", &attr);
    if (theTimingModels.count(thePhysMemory) > 0) {
      DBG_Assert( false, ( << "Two CPUs connected to the same memory timing_model: " << thePhysMemory->name) );
    }
    theTimingModels.insert(thePhysMemory);

#if FLEXUS_TARGET_IS(v9)
    //We only use the snoop interface for v9
    /* Tell memory we have a mem hier */
    Simics::API::SIM_set_attribute(thePhysMemory, "snoop_device", &attr);

    /* Tell memory we have a mem hier */
    thePhysIO = Simics::API::SIM_get_attribute(theCPU, "physical_io").u.object;
    Simics::API::SIM_set_attribute(thePhysIO, "timing_model", &attr);

    if (theTimingModels.count(thePhysIO) > 0) {
      DBG_Assert( false, ( << "Two CPUs connected to the same I/O timing_model: " << thePhysIO->name) );
    }
    theTimingModels.insert(thePhysIO);

#else //! FLEXUS_TARGET_IS(v9)
    thePhysIO = 0;
#endif //FLEXUS_TARGET_IS(v9)

    current_pc = 0;

  }

  void setTraceConsumer(std::shared_ptr<SimicsTraceConsumer> aConsumer) {
    theConsumer = aConsumer;
    theConsumer->init(theIndex);
  }

  void setCycleManager(SimicsCycleManager & aManager) {
    theCycleManager = &aManager;
  }

  void setIgnore() {
    DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "set ignore" ) );
    theIgnoreFlag = true;
  }

  void clearIgnore() {
    DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "clear ignore" ) );
    theIgnoreFlag = false;
  }

  bool ignore() const {
    return theIgnoreFlag;
  }

  void enableInterrupts() {
    theInterruptsEnabled = true;
    Simics::API::attr_value_t attr;
    attr.kind = Simics::API::Sim_Val_Integer;
#if FLEXUS_TARGET_IS(v9)
    attr.u.integer = 1;
    Simics::API::SIM_set_attribute(theCPU, "extra_irq_enable", &attr);
#elif FLEXUS_TARGET_IS(x86)
    attr.u.integer = 0;
    Simics::API::SIM_set_attribute(theCPU, "temporary_interrupt_mask", &attr);
#endif //FLEXUS_TARGET_IS(v9)
  }

  void disableInterrupts() {
    theInterruptsEnabled = false;
    Simics::API::attr_value_t attr;
    attr.kind = Simics::API::Sim_Val_Integer;
#if FLEXUS_TARGET_IS(v9)
    attr.u.integer = 0;
    Simics::API::SIM_set_attribute(theCPU, "extra_irq_enable", &attr);
#elif FLEXUS_TARGET_IS(x86)
    attr.u.integer = 1;
    Simics::API::SIM_set_attribute(theCPU, "temporary_interrupt_mask", &attr);
#endif //FLEXUS_TARGET_IS(v9)
  }

  bool interruptsEnabled() const {
    return theInterruptsEnabled;
  }

  DoubleWord readStoreBuffer(PhysicalMemoryAddress const & anAlignedAddress) {
    StoreBuffer::iterator iter = theStoreBuffer.find(anAlignedAddress);
    if (iter != theStoreBuffer.end()) {
      //DBG_(Tmp, ( << "Store buffer access hit for address: " << &std::hex << anAlignedAddress << &std::dec << " returning value: " << iter->second.theNewValue ));
      return iter->second.theNewValue;
    }
    return DoubleWord();
  }

  //Returns the current contents of the store buffer for a memory location
  DoubleWord getMemoryValue(PhysicalMemoryAddress & anAlignedAddress) {
    uint64_t memory( Simics::API::SIM_read_phys_memory(theCPU, anAlignedAddress, 8) );
    DoubleWord store_buffer( readStoreBuffer(anAlignedAddress) );
    DoubleWord ret_val(memory, store_buffer);
    DBG_(VVerb, ( << "Applying SB: " << store_buffer << " to " << & std::hex << memory << & std::dec << " results in " << ret_val << &std::dec  ));
    return ret_val;
  }

  Simics::API::cycles_t trace_mem_hier_operate(Simics::API::conf_object_t * space, Simics::API::map_list_t * map, Simics::API::generic_transaction_t * aMemTrans) {
    Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Simics::APIFwd::memory_transaction_t *>(aMemTrans);
#ifdef FLEXUS_FEEDER_TRACE_DEBUGGER
    theConsumer->debugger.nextCallback(mem_trans);
#endif //FLEXUS_FEEDER_TRACE_DEBUGGER

    int32_t stall = real_hier_operate(space, map, mem_trans);

#ifdef FLEXUS_FEEDER_TRACE_DEBUGGER
    theConsumer->debugger.ret(stall);

    // verify we aren't trying to stall when it is not permitted
    if ( (!mem_trans->s.may_stall) && (stall > 0) ) {
      DBG_(Crit, SetNumeric( (FlexusIdx) theIndex )
           ( << "stalling " << stall << " cycles when not permitted" ) );
      theConsumer->debugger.dump();
    }
#endif //FLEXUS_FEEDER_TRACE_DEBUGGER

    if (theStallCap > 0 && stall > theStallCap) {
      stall = theStallCap;
    }

    return stall;
  }

  //Useful debugging stuff for tracing every instruction
  void debugTransaction(Simics::APIFwd::memory_transaction_t * mem_trans) {
    Simics::API::logical_address_t pc_logical = Simics::API::SIM_get_program_counter(theCPU);
    Simics::API::physical_address_t pc = Simics::API::SIM_logical_to_physical(theCPU, Simics::API::Sim_DI_Instruction, pc_logical);
    if (Simics::API::SIM_clear_exception() != Simics::API::SimExc_No_Exception) {
      DBG_(Tmp, SetNumeric( (FlexusIdx) theIndex)
           ( << "Instruction results in ITLB Miss pc_logical: " << pc_logical)
          );
      return;
    }
    Simics::API::tuple_int_string_t * retval = Simics::API::SIM_disassemble(theCPU, pc , 0);
    (void)retval; //suppress unused variable warning

    DBG_(Tmp, SetNumeric( (FlexusIdx) theIndex)
         ( << "Mem Heir Instr: " << retval->string << " pc: " << pc)
        );

    if (Simics::API::SIM_mem_op_is_data(&mem_trans->s)) {
      if (Simics::API::SIM_mem_op_is_write(&mem_trans->s)) {
        DBG_(Tmp, SetNumeric( (FlexusIdx) theIndex)
             ( << "  Write @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size << ']'
               << " type=" << mem_trans->s.type
               << (  mem_trans->s.atomic ? " atomic" : "" )
               << (  mem_trans->s.may_stall ? "" : " no-stall" )
               << (  mem_trans->s.inquiry ? " inquiry" : "")
               << (  mem_trans->s.speculative ? " speculative" : "")
               << (  mem_trans->s.ignore ? " ignore" : "")
             )
            );
      } else {
        if ( mem_trans->s.type == Simics::API::Sim_Trans_Prefetch) {
          DBG_(Tmp, SetNumeric( (FlexusIdx) theIndex)
               ( << "  Prefetch @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size  << ']'
                 << " type=" << mem_trans->s.type
                 << (  mem_trans->s.atomic ? " atomic" : "" )
                 << (  mem_trans->s.may_stall ? "" : " no-stall" )
                 << (  mem_trans->s.inquiry ? " inquiry" : "")
                 << (  mem_trans->s.speculative ? " speculative" : "")
                 << (  mem_trans->s.ignore ? " ignore" : "")
               )
              );
        } else {
          DBG_(Tmp, SetNumeric( (FlexusIdx) theIndex)
               ( << "  Read @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size  << ']'
                 << " type=" << mem_trans->s.type
                 << (  mem_trans->s.atomic ? " atomic" : "" )
                 << (  mem_trans->s.may_stall ? "" : " no-stall" )
                 << (  mem_trans->s.inquiry ? " inquiry" : "")
                 << (  mem_trans->s.speculative ? " speculative" : "")
                 << (  mem_trans->s.ignore ? " ignore" : "")
               )
              );
        }
      }
    }
  }

  bool requiresSync(Simics::APIFwd::memory_transaction_t * mem_trans) {
    if ( mem_trans->s.size > 8) {
      //Anything larger than 8 bytes must sync
      return true;
    }

#if FLEXUS_TARGET_IS(v9)
    //If we are using a stange ASI, mark this as a sync
    switch ( mem_trans->address_space ) {
        //Privileged
      case 0x04: //NUCLEUS
      case 0x0C: //NUCLEUS_LITTLE
      case 0x10: //AS_IF_USER_PRIMARY
      case 0x11: //AS_IF_USER_SECONDARY
      case 0x18: //AS_IF_USER_PRIMARY_LITTLE
      case 0x19: //AS_IF_USER_SECONDARY_LITTLE
      case 0x24: //NUCLEUS_QUAD_LDD
      case 0x2C: //NUCLEUS_QUAD_LDD_LITTLE
        //User
      case 0x81: //SECONDARY
      case 0x88: //PRIMARY_LITTLE
      case 0x89: //SECONDARY_LITTLE
      case 0x80: //PRIMARY
        break;
      default:
        //Any other ASI requires a Sync
        DBG_(Iface, SetNumeric( (FlexusIdx) theIndex)
             (    << "Alternate ASI " << mem_trans->address_space << " @" << &std::hex
                  << mem_trans->s.physical_address
                  << '[' << &std::dec << mem_trans->s.size << ']'
             )
            );

        return true;
    }

    //Non-cacheable operations require a sync
    if ( (! mem_trans->cache_virtual) || ( ! mem_trans->cache_physical) ) {
      DBG_(Iface, SetNumeric( (FlexusIdx) theIndex)
           (    << "Non-cacheable @" << &std::hex
                << mem_trans->s.physical_address
                << '[' << &std::dec << mem_trans->s.size << ']'
           )
          );
      return true;
    }
#endif //FLEXUS_TARGET_IS(v9)

    return false;
  }

  Simics::API::cycles_t real_hier_operate(Simics::API::conf_object_t * space, Simics::API::map_list_t * map, Simics::APIFwd::memory_transaction_t * mem_trans) {
    //NOTE: Multiple return paths
    const int32_t k_no_stall = 0;
    const int32_t k_call_me_next_cycle = 1;

    //debugTransaction(mem_trans);

    //Ensure that we see future accesses to this block
    mem_trans->s.block_STC = 1;

    //Case 0: This is an IO access
    //============================

    //All IO accesses must stall until SB is empty
    if (thePhysIO != 0 && space == thePhysIO) {
      if ( ! theStoreBuffer.empty() ) {
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  Got an IO access while the Store buffer contains data.  Must stall access till the store buffer flushes." ) );

        // JWK:
        // Previous IO (store) instruction should be marked as IO instruction here...
        // Execute doesn't release it until SB becomes empty
        // This instruction must be the last one in theEntries....
        theConsumer->theEntries.back().theReadyInstruction->setIO();

        theCycleManager->advanceFlexus();
        Simics::API::cycles_t stall_cycles = theCycleManager->reconcileTime(theConsumer->queueSize());

        if (stall_cycles == 0) {
          disableInterrupts();
          return k_call_me_next_cycle;
        } else {
          disableInterrupts();
          return stall_cycles;
        }
      } else {
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  Got an IO access but store buffer is empty.  Access may proceed." ) );
        //Otherwise, we allow the access to complete.
        return k_no_stall;
      }

    }

    //Case 1: Previously pending operation has completed since last call
    //==================================================================
    //See if perviously pending instructions have been completed
    if (theConsumer->isComplete()) {
      // We have completed the instruction that was previously stalled since
      // the last time we were called by Simics

      DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 1: completed operation on " << Simics::API::SIM_current_processor()->name ) );

#if FLEXUS_TARGET_IS(v9)
      if ( mem_trans->s.type == Simics::API::Sim_Trans_Instr_Fetch ) {
        DBG_Assert( current_pc == mem_trans->s.physical_address ) ;
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Ignoring re-issue of fetch operation." ) );
        return 0;
      }
#else //x86
      if ( x86_interrupt_problem == 0) {
        if ( mem_trans->s.type == Simics::API::Sim_Trans_Instr_Fetch ) {
          DBG_Assert( current_pc == mem_trans->s.physical_address ) ;
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Ignoring re-issue of fetch operation." ) );
          return 0;
        }
      }
#endif

      // Indicate that Simics is aware the instruction is complete
      theConsumer->simicsDone();

      //If we have placed this store instruction in the store buffer, then
      //we should prevent simics from performing the store operation now
      if (ignore()) {
        //Clear the ignore flag
        clearIgnore();

        //DBG_Assert( ( mem_trans->s.type == Sim_Trans_Store ) /*|| ( mem_trans->s.type == Sim_Trans_Load )*/);

        if ( ( mem_trans->s.type == Simics::API::Sim_Trans_Store ) ) {

          //Tell Simics not to perform this store operation
          mem_trans->s.ignore = 1;
          DBG_( VVerb, ( << "Ignoring this operation" ) );
        } else {
          DBG_( Crit, SetNumeric( (FlexusIdx) theIndex ) ( << "Expected to ignore a store completion, but got something that isn't a store.  Transaction follows: " ) );
          debugTransaction(mem_trans);
        }
      }

      //We are completing the current instruction, so interrupts are ok
      enableInterrupts();

#if FLEXUS_TARGET_IS(x86)
      if (x86_interrupt_problem) {
        x86_interrupt_problem = 0;
        return k_call_me_next_cycle;
      } else { // return zero cycle latency
        return k_no_stall;
      }
#else //v9
      // return zero cycle latency
      return k_no_stall;
#endif
    }

    //All the code below needs to distinguish fetches from data operations
    bool is_fetch = ( mem_trans->s.type == Simics::API::Sim_Trans_Instr_Fetch);
    bool is_data = ( mem_trans->s.type == Simics::API::Sim_Trans_Load ) || ( mem_trans->s.type == Simics::API::Sim_Trans_Store ) || ( mem_trans->s.type == Simics::API::Sim_Trans_Prefetch);

    //Case 2: We have a pending data operation that has not been completed
    //====================================================================
    // See if we have a pending operation.
    if (! theConsumer->isIdle()) {
      //There is some operation pending for this cpu, and it has not yet
      //been completed by flexus.

      DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 2: incomplete pending op on " << Simics::API::SIM_current_processor()->name ) );

#if FLEXUS_TARGET_IS(x86)
      //Check to see if that problem with the x86 interrupts happens right now!!
      if (mem_trans->s.may_stall == 0) {
        //we assume that the interrupt problem is in progress. The instructions of the interrupt handler come
        //in and we cannot stall them. Also the last "user" instruction is pending in the memory hierarchy.
        //This shouldn't have happened if the "cpuX.temporary_interrupt_mask" flag was working properly.
        //Set a flag to remember that there is a pending instruction. When the interrupt handler completes
        //the next instruction will be stalled until the last "user" one completes.
        x86_interrupt_problem = 1;
        ++x86_interrupt_problem_counter;

        DBG_( Tmp, SetNumeric( (FlexusIdx) theIndex ) ( << "Interrupt handler instruction ignored. "
              << "So far " << x86_interrupt_problem_counter
              << " instructions have been ignored on "
              << Simics::API::SIM_current_processor()->name)
            );

        return 0;
      }

      //If the interrupt handler's instructions are over then stall this instruction
      //until flexus becomes available
      if ((x86_interrupt_problem == 1) & (mem_trans->s.may_stall == 1)) {
        return k_call_me_next_cycle;
      }
#endif

      //First, some assertions to make sure Simics has given us back the
      //same transaction as we were stalled on previously.

      if ( is_fetch ) {
        DBG_Assert( current_pc == 0 || current_pc == mem_trans->s.physical_address, ( << "CPU" << theIndex << " current_pc=" << current_pc << " mem_trans->s.physical_address=" << mem_trans->s.physical_address)) ;
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Ignoring re-issue of instruction fetch" ) );
        return 0;
      } else if ( is_data ) {
        //Ensure that this data operation is going t
        if ( PhysicalMemoryAddress(mem_trans->s.physical_address) !=
             theConsumer->optimizedGetInstruction().physicalMemoryAddress() ) {
          DBG_( Crit, SetNumeric( (FlexusIdx) theIndex )
                ( << "data addresses don't match for reissued Simics request. Expected Instr: "
                  << theConsumer->optimizedGetInstruction()
                  << " Received: "
                  << mem_trans->s.physical_address
                ) );
#ifdef FLEXUS_FEEDER_TRACE_DEBUGGER
          theConsumer->debugger.dump();
#endif //FLEXUS_FEEDER_TRACE_DEBUGGER
        }
      } else {
        DBG_(Crit, SetNumeric( (FlexusIdx) theIndex )
             ( << "while pending, got a new transaction that is neither inst nor data" ) );
      }

      //Interrupts should be disabled
      DBG_Assert( ! interruptsEnabled() ) ;

#ifndef FLEXUS_FEEDER_OLD_SCHEDULING

      // Try to advance Flexus
      theCycleManager->advanceFlexus();
      Simics::API::cycles_t stall_cycles = theCycleManager->reconcileTime(theConsumer->queueSize());

      if (stall_cycles > 0) {
        //All pending operations have not yet completed.
        //Interrupts remain disabled.  We will be called again.
        return stall_cycles;
      } else {
        //We can not advance flexus further.  This is either because
        //this cpu has completed all its pending operations, or because
        //some other cpu has run out of instructions.

        if (theConsumer->isComplete()) {
          //This cpu has completed all its operations.

          //Check if we should prevent Simics from executing the
          //operation
          if (ignore()) {
            //We should.  This should only be the case for store
            //instructions
            DBG_Assert( ( mem_trans->s.type == Simics::API::Sim_Trans_Store )    );

            //Inform Simics to ignore the store
            mem_trans->s.ignore = 1;

            //Clear the store pending flag
            clearIgnore();
            DBG_( VVerb, ( << "Ignoring this store" ) );
          }

          //Indicate to the consumer that we have notified simics that
          //the instruction is complete
          theConsumer->simicsDone();

          enableInterrupts();
          return k_no_stall;
        } else {
          //Some cpu has run out of instructions, but this cpu is still
          //pending. Interrupts remain disabled.  We will be called again.
          return k_call_me_next_cycle;  //Call back next cycle
        }
      }

#else //FLEXUS_FEEDER_OLD_SCHEDULING
      return k_call_me_next_cycle;  //Call back next cycle
#endif //FLEXUS_FEEDER_OLD_SCHEDULING

    }

    //Case 3: We have a new instruction fetch
    //=======================================
    if ( is_fetch ) {
      DBG_(VVerb,  SetNumeric( (FlexusIdx) theIndex ) ( << "Case 3: new instruction fetch on " << Simics::API::SIM_current_processor()->name) );

      if (theFlexus->quiescing() ) {
        //We halt new instruction fetches when we are trying to quiesce Flexus
        return k_call_me_next_cycle;
      }

      // Instruction fetches should never be atomic memory operations
      DBG_Assert(!mem_trans->s.atomic, SetNumeric( (FlexusIdx) theIndex) );

      //If we have many instructions queued, we will advance Flexus
      //to drain the queue.  This may result in stall cycles.  However,
      //by default, we return without stall

      Simics::API::cycles_t ret = k_no_stall;

#ifndef FLEXUS_FEEDER_OLD_SCHEDULING
      if (theConsumer->largeQueue()) {
        //theConsumer has many instructions queued.  We advance.

        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "advancing large queue" ) );
        theCycleManager->advanceFlexus();

        //We give reconcileTime the number of pending instructions plus
        //1 for the instruction we have not yet queued.
        ret = theCycleManager->reconcileTime(theConsumer->queueSize() + 1); //Take the instruction that we have not yet queued into account

      }
#endif

#if FLEXUS_TARGET_IS(x86)
      int32_t is_hlt = 0;
      if (mem_trans->s.may_stall == 0) {
        uint32_t opcode_1byte = SIM_read_phys_memory(theCPU, mem_trans->s.physical_address, 1);
        if (opcode_1byte == 244) { //its a 'hlt' instruction !!!
          is_hlt = 1;
          if (theConsumer->queueSize() > 0) {
            DBG_(VVerb, ( << "Discarding HALT instruction. Opcode = " << std::hex << opcode_1byte));
            enableInterrupts();
            return 0;
          } else {
            DBG_(VVerb, ( << "It's ok to enqueue a 'hlt' instruction for now. Queue size = " << theConsumer->queueSize()));
          }
        }
      }
#endif

      // create a new instruction object
      intrusive_ptr<ArchitecturalInstruction> new_inst( new ArchitecturalInstruction(theConsumer.get()) ) ;

      // set the PC
      new_inst->setVirtInstAddress( VirtualMemoryAddress(mem_trans->s.logical_address) );
      new_inst->setPhysInstAddress( PhysicalMemoryAddress(mem_trans->s.physical_address) );

      new_inst->setInstructionSize((char)mem_trans->s.size);

#if FLEXUS_TARGET_IS(x86)
      if (is_hlt == 1)
        new_inst->setIfPart(Halt_instr);
#endif

      current_pc = mem_trans->s.physical_address;

#if FLEXUS_TARGET_IS(v9)
      if (mem_trans->priv) {
        //Mark privileged operations
        new_inst->setPriv();
      }

      //See if the instruction is a MEMBAR
      uint32_t op_code = SIM_read_phys_memory(theCPU, mem_trans->s.physical_address, 4);
      new_inst->setOpcode(op_code);

      //MEMBAR  is 1000 0001 0100 0011 11-- ---- ---- ----
      const uint32_t kMEMBAR_mask = 0xFFFFC000;
      const uint32_t kMEMBAR_pattern = 0x8143C000;
      if ((op_code & kMEMBAR_mask) == kMEMBAR_pattern) {
        //It is a MEMBAR. Figure out which MEMBAR it is.  We care about:
        //MEMBAR #sync            8143E0040
        //MEMBAR #memissue        8143E0020
        //MEMBAR #lookaside       8143E0010
        //MEMBAR #storeload       8143E0002
        const uint32_t kMEMBAR_SYNC_mask = 0x72;

        if ( (op_code & kMEMBAR_SYNC_mask) != 0) {
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  Instruction is a syncing MEMBAR" ) );
          new_inst->setIsMEMBAR();
        }

      }
#else
      if (mem_trans->mode == Simics::API::Sim_CPU_Mode_Supervisor) {
        //Mark privileged operations
        new_inst->setPriv();
      }

      //Reading op-codes not supported on x86
      new_inst->setOpcode(0);
#endif //FLEXUS_TARGET_IS(v9)

      //Pass the newly created instruction to the consumer
      theConsumer->consumeInstOperation(new_inst);

      //If we did not stall while advancing, or did not adance
      if (ret == 0) {
        enableInterrupts();
      } else {
        disableInterrupts();
      }
      return ret;

    }

    //Case 4: We have a data operation to attach to the previously fetched instruction
    //================================================================================
    if ( is_data ) {
      DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 4: Data operation" ) );

      bool is_write = ( mem_trans->s.type == Simics::API::Sim_Trans_Store );
      bool is_atomic = mem_trans->s.atomic;

      //Obtain the physical PC of this operation so we can verify
      //that it matches the first half of the atomic operation
      Simics::API::logical_address_t pc_logical = Simics::API::SIM_get_program_counter(theCPU);
      Simics::API::physical_address_t pc = Simics::API::SIM_logical_to_physical(theCPU, Simics::API::Sim_DI_Instruction, pc_logical);
      if (Simics::API::SIM_clear_exception() != Simics::API::SimExc_No_Exception) {
        pc = 0;
      }

      //Case 4.1: Atomic memory operation
      //=================================

      // for atomic operations, if this is the first half (the read), just
      // append the data reference onto the most recent instruction; if this
      // is the second half, verify that it matches with the first half
      if ( is_atomic ) {

        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 4.1: Atomic operation" ) );

#if FLEXUS_TARGET_IS(v9)
        //v9 implementation - handles special cases where simics
        //issues the pieces of RMWs separately

        //See if this is the first or second half of the operation.  It
        //is the second half if theConsumer's queue is empty, since this
        //means we just got a second back-to-back data operation, which
        //only occurs in the case of the second mem ops
        if (theConsumer->theEntries.empty() && theConsumer->isAtomicOperationPending()) {
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  second half of atomic operation" ) );

          //Verify that the second half of the atomic operation matches
          //the first half that we just completed
          theConsumer->verifyAtomicOperation(is_write, PhysicalMemoryAddress(pc), PhysicalMemoryAddress(mem_trans->s.physical_address ) );

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex )
               ( << "  verified atomic operation" ) );

          enableInterrupts();
          return k_no_stall;
        } else {
          //First half of an atomic operation
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  first half of atomic operation" ) );

          //Better not be a write
          DBG_Assert(!is_write);

          // Obtain a reference to the previous fetch so we can add the data
          // operation to it.
          ArchitecturalInstruction & inst = theConsumer->optimizedGetInstruction();

          //Ensure that we are not overwriting another instruction
          DBG_Assert(inst.isNOP());

          //Fill in the physical & virtual address for this memory operation
          inst.setAddress( PhysicalMemoryAddress(mem_trans->s.physical_address) );
          inst.setVirtualAddress( VirtualMemoryAddress(mem_trans->s.logical_address));

          // record the opcode
          uint32_t op_code = SIM_read_phys_memory(theCPU, pc, 4);
          inst.setOpcode(op_code);

          //LDD(a)            is 11-- ---0 -001 1--- ---- ---- ---- ----
          //STD(a)            is 11-- ---0 -011 1--- ---- ---- ---- ----

          //LDSTUB(a)/SWAP(a) is 11-- ---0 -11- 1--- ---- ---- ---- ----
          //CAS(x)A           is 11-- ---1 111- 0--- ---- ---- ---- ----

          const uint32_t kLDD_mask = 0xC1780000;
          const uint32_t kLDD_pattern = 0xC0180000;

          const uint32_t kSTD_mask = 0xC1780000;
          const uint32_t kSTD_pattern = 0xC038000;

          const uint32_t kRMW_mask = 0xC1680000;
          const uint32_t kRMW_pattern = 0xC0680000;

          const uint32_t kCAS_mask = 0xC1E80000;
          const uint32_t kCAS_pattern = 0xC1E00000;

          if ((op_code & kLDD_mask) == kLDD_pattern) {
            inst.setIsLoad();
          } else if ((op_code & kSTD_mask) == kSTD_pattern) {
            inst.setIsStore();
            inst.setSync();
          } else if (((op_code & kRMW_mask) == kRMW_pattern) || ((op_code & kCAS_mask) == kCAS_pattern))  {
            inst.setIsRmw();
          } else {
            DBG_Assert( false, ( << "Unknown atomic operation. Opcode: " << std::hex << op_code << " pc: " << pc << std::dec ) );
          }

          theConsumer->recordAtomicVerification( PhysicalMemoryAddress(pc), PhysicalMemoryAddress(mem_trans->s.physical_address ));

          //Remember the size of the rmw
          inst.setSize(mem_trans->s.size);

          if (mem_trans->priv) {
            //Mark privileged operations
            inst.setPriv();
          }

          // add this data instruction to the consumer
          theConsumer->consumeDataOperation();

        }
#else //Not v9

        //See if this is a second (or subsequent) part of an atomic
        //memory access that we have already passed to the consumer
        //(as an RMW instruction).  If the last thing we passed the
        //consumer was an atomic memory transaction with the same PC
        //and data address, we assume that this new transaction is the
        //same instruction as the previous one.  However, if there
        //is a mismatch between data and PC, we issue it to flexus as
        //a separate RMW.  This ensures that we get the right memory
        //accesses in Flexus.

        if (  theConsumer->isAtomicOperationPending()
              && theConsumer->getAtomicPC() == PhysicalMemoryAddress(pc)
           ) {
          //This memory transaction completes an atomic operation that
          //has already been sent to the consumer.  No need to send
          //another operation through.

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Second half of atomic operation" ) );
          theConsumer->clearAtomicPending();
          enableInterrupts();
          return k_no_stall;

        } else {
          //This does not match an atomic operation we just issued.
          //Issue it as a new atomic operation

          DBG_(VVerb,  SetNumeric( (FlexusIdx) theIndex ) ( << "  first half of atomic operation" ) );
          //Construct a new instruction object, if neccessary
          if ( theConsumer->isEmpty() ) {
            // create a new instruction object
            intrusive_ptr<ArchitecturalInstruction> new_inst( new ArchitecturalInstruction( theConsumer.get() ) ) ;
            // set the PC
            new_inst->setPhysInstAddress( PhysicalMemoryAddress( pc ) );
            //Pass the newly created instruction to the consumer
            theConsumer->consumeInstOperation(new_inst);
          }

          // Obtain a reference to the previous fetch so we can add the data
          // operation to it.
          ArchitecturalInstruction & inst = theConsumer->optimizedGetInstruction();

          //Fill in the physical address for this memory operation
          inst.setAddress( PhysicalMemoryAddress(mem_trans->s.physical_address) );

          //Fill in the virtual address for this memory operation
          inst.setVirtualAddress( VirtualMemoryAddress(mem_trans->s.logical_address));

          //Assume all atomic x86 operations are RMWs.  This may not
          //be true
          inst.setIsRmw();

          //Remember the size of the load or store
          inst.setSize(mem_trans->s.size);

          theConsumer->recordAtomicVerification( PhysicalMemoryAddress(pc), PhysicalMemoryAddress(mem_trans->s.physical_address ));

          if (mem_trans->mode == Simics::API::Sim_CPU_Mode_Supervisor) {
            //Mark privileged operations
            inst.setPriv();
          }

          // add this data instruction to the consumer
          theConsumer->consumeDataOperation();

        }

#endif //FLEXUS_TARGET_IS(v9)

        //Case 4.2: Load or store operation
        //=================================
      } else {

        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 4.2: Load or Store operation" ) );

        if ( theConsumer->isEmpty() ) {

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 4.2a: non-stallable LDD or STD" ) );

          //Non-stallable LDD and STD instructions are the only cases where
          //we can have a memory operation while theConsumer is empty

          //We simulate a fetch operation here.
          //Need to get a PC.
          Simics::API::logical_address_t pc_logical = Simics::API::SIM_get_program_counter(theCPU);
          Simics::API::physical_address_t pc = Simics::API::SIM_logical_to_physical(theCPU, Simics::API::Sim_DI_Instruction, pc_logical);
          if (Simics::API::SIM_clear_exception() != Simics::API::SimExc_No_Exception) {
            pc = 0;
          }

          // create a new instruction object
          intrusive_ptr<ArchitecturalInstruction> new_inst( new ArchitecturalInstruction( theConsumer.get() ) ) ;

          // set the PC
          new_inst->setPhysInstAddress( PhysicalMemoryAddress( pc ) );

          new_inst->setInstructionSize((char)mem_trans->s.size);

#if FLEXUS_TARGET_IS(x86)
          //This instruction instance carries the second part of an instruction that performs two memory operations.
          //Thus, it shouldn't perform I-fetch.
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "The consumer is empty"));
          if (mem_trans->access_type == Flexus::Simics::API::X86_Vanilla) {
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "Creating an instruction for a normal memory access"));
            new_inst->setIfPart(Second_MemPart);
          } else { //or we just got a hardware walk memory access
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "Creating an instruction for a hardware walk memory access"));
            new_inst->setIfPart(Hardware_Walk);
          }
#endif

          //Pass the newly created instruction to the consumer
          theConsumer->consumeInstOperation(new_inst);

        }
#if FLEXUS_TARGET_IS(x86)
        else if ((mem_trans->access_type == Flexus::Simics::API::X86_Idt)  ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Gdt)  ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Ldt)  ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Pml4) ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Pdp)  ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Pd) ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Task_Segment) ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Stack) ||
                 (mem_trans->access_type == Flexus::Simics::API::X86_Pt) ) {

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "Received a memory operation that requires special attention"
               << " (hardware walk). Create a new instruction instance"));

          Simics::API::physical_address_t pc = 0;
          // create a new instruction object
          intrusive_ptr<ArchitecturalInstruction> new_inst( new ArchitecturalInstruction( theConsumer.get() ) ) ;

          // set the PC
          new_inst->setPhysInstAddress( PhysicalMemoryAddress( pc ) );

          new_inst->setInstructionSize((char)mem_trans->s.size);

          //This instruction carries the memory access of hardware walk operation.
          //Thus, it shouldn't perform I-fetch.
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "Creating an instruction for a hardware walk memory access"));
          new_inst->setIfPart(Hardware_Walk);

          //Pass the newly created instruction to the consumer
          theConsumer->consumeInstOperation(new_inst);

        }
#endif

        // Obtain a reference to the previous fetch so we can add the data
        // operation to it.
        ArchitecturalInstruction & inst = theConsumer->optimizedGetInstruction();

        //Fill in the physical address for this memory operation
        inst.setAddress( PhysicalMemoryAddress(mem_trans->s.physical_address) );

        //Fill in the virtual address for this memory operation
        inst.setVirtualAddress( VirtualMemoryAddress(mem_trans->s.logical_address));

        if (is_write) {
          //Indicate that its a store
          inst.setIsStore();
        } else {
          inst.setIsLoad();
        }

        //See if it meets any of the conditions which require a sync
        if ( requiresSync(mem_trans) ) {
          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  store requires sync" ) );
          inst.setSync();
        }

        //See if this is a store operation which may use the store buffer
        if (is_write && !inst.isSync() && FLEXUS_TARGET_IS(v9) ) {

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "Case 4.2b: Store which uses Store Buffer" ) );

          //This store does not require a Sync, so it may use the store
          //buffer

          //Obtain the double-word-aligned address
          PhysicalMemoryAddress aligned_addr(mem_trans->s.physical_address & ~7LL);

          //Construct the new word-aligned value
          DoubleWord new_value;
          new_value.set( SIM_get_mem_op_value_cpu(&mem_trans->s), mem_trans->s.size, (mem_trans->s.physical_address & 7));

          DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) Addr(mem_trans->s.physical_address) ( << "  SB-entry Store @" << &std::hex << mem_trans->s.physical_address << '[' << &std::dec << mem_trans->s.size << "] aligned: " << &std::hex << aligned_addr << &std::dec << " new value: " << new_value) );

          //Prevent mem op from modifying memory
          setIgnore();
          //enter the memory op into the store buffer.
          std::pair<StoreBuffer::iterator, bool> entry =
            theStoreBuffer.insert
            (  std::make_pair
               ( aligned_addr
                 , StoreBufferEntry( new_value )
               )
            );

          //Already had an entry, coalescing
          if (! entry.second) {
            //Increment the outstanding store count
            ++( entry.first->second);

            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex ) ( << "  coalescing with existing entry" ) );

            //Change the new value
            entry.first->second.theNewValue.set( SIM_get_mem_op_value_cpu(&mem_trans->s), mem_trans->s.size, (mem_trans->s.physical_address & 7) ) ;
          }

          //Attach the store buffer to the instruction
          inst.setStoreBuffer(&theStoreBuffer);

          //Remember the data and size of the store, so we can perform
          //it later
          inst.setData(new_value);
        }

        //Remember the size of the load or store
        inst.setSize(mem_trans->s.size);

#if FLEXUS_TARGET_IS(v9)
        if (mem_trans->priv) {
          //Mark privileged operations
          inst.setPriv();
        }
#elif FLEXUS_TARGET_IS(x86)
        if (mem_trans->mode == Simics::API::Sim_CPU_Mode_Supervisor) {
          //Mark privileged operations
          inst.setPriv();
        }
#endif //FLEXUS_TARGET_IS(v9)

        // add this data instruction to the consumer
        theConsumer->consumeDataOperation();
      }

      //Advance Flexus for sub-cases of Case 4
      //========================================

      if ( ! mem_trans->s.may_stall ) {
        //We complete the STD / LDD without advancing flexus

        //See if the store operation should be supressed
        if (ignore()) {

          mem_trans->s.ignore = 1;
          clearIgnore();
          DBG_( VVerb, ( << "  ignoring this op" ) );
        }

        theConsumer->simicsDone();

        enableInterrupts();
        // return zero cycle latency
        return k_no_stall;
      } else {

#ifndef FLEXUS_FEEDER_OLD_SCHEDULING
        //Advance flexus and determine if we must stall
        theCycleManager->advanceFlexus();
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex )
             ( << "  Advancing flexus" ) );
        Simics::API::cycles_t stall_cycles = theCycleManager->reconcileTime(theConsumer->queueSize());

        if (stall_cycles > 0) {
          //We must stall, so we will have an operation pending.  Disable
          //interrupts so Simics doesn't change the operation on us
          disableInterrupts();
          return stall_cycles;
        } else {
          //No need to stall
          if (theConsumer->isComplete()) {
            //We finished the pending operation

            //See if the store operation should be supressed
            if (ignore()) {
              mem_trans->s.ignore = 1;
              DBG_Assert( ( mem_trans->s.type == Simics::API::Sim_Trans_Store )    );
              clearIgnore();
              DBG_( VVerb, ( << "  ignoring this op" ) );
            }

            //Indicate that we are done
            theConsumer->simicsDone();

            //Allow interrupts again.
            enableInterrupts();
            return k_no_stall;
          } else {
            //We must stall, so we will have an operation pending.  Disable
            //interrupts so Simics doesn't change the operation on us
            disableInterrupts();
            return k_call_me_next_cycle;  //Call back next cycle
          }
        }
#else //defined(FLEXUS_FEEDER_OLD_SCHEDULING)
        disableInterrupts();
        return k_call_me_next_cycle;  //Call back next cycle
#endif //FLEXUS_FEEDER_OLD_SCHEDULING
      }
    }
    //Case 5: We have something that is neither instruction, nor data. We simply
    //complete these without taking any action in flexus
    //================================================================================
    //Neither instruction nor data access.  We will ignore this.
    /* shouldn't happen? */
    DBG_(Crit, SetNumeric( (FlexusIdx) theIndex) ( << "Case 5: Neither instruction nor data" ) );

    clearIgnore();
    enableInterrupts();

    return k_no_stall;
  }

  Simics::API::cycles_t trace_snoop_operate(Simics::API::conf_object_t * space, Simics::API::map_list_t * map, Simics::API::generic_transaction_t * aMemTrans) {

    Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Simics::APIFwd::memory_transaction_t *>(aMemTrans);

    if (Simics::API::SIM_mem_op_is_data(&mem_trans->s) && (mem_trans->s.size <= 8) ) {
      //We do not snoop PREFETCH and block load/store operations

      //Obtain the word-aligned address
      PhysicalMemoryAddress aligned_addr(mem_trans->s.physical_address & ~7LL);

      DBG_(VVerb, Condition(Simics::API::SIM_mem_op_is_write(&mem_trans->s))
           SetNumeric( (FlexusIdx) theIndex)
           (  << "Snoop interface Write @"
              << &std::hex  << mem_trans->s.physical_address
              << " aligned: " << aligned_addr << &std::dec
           )
          );

      PhysicalMemoryAddress interesting_region(mem_trans->s.physical_address & ~0xFFLL);

      //If this transaction is a store, assert that we have a store buffer enty
      //for it
      if (! Simics::API::SIM_mem_op_is_write(&mem_trans->s)) {
        //Assert that Simics got what we think is the new value

        DoubleWord value_according_to_sb(getMemoryValue(aligned_addr));
        DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << "    Value including SB contents: " << value_according_to_sb ) );

        //The correct value is in the store buffer
        switch ( mem_trans->s.size) {
          case 1: {
            uint8_t our_value = value_according_to_sb.getByte(mem_trans->s.physical_address & 7);
            SIM_set_mem_op_value_cpu(&mem_trans->s, our_value);
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << " snoop value(1): " << &std::hex << (uint32_t)our_value) );
          }
          break;
          case 2: {
            uint16_t our_value = value_according_to_sb.getHalfWord(mem_trans->s.physical_address & 7);
            SIM_set_mem_op_value_cpu(&mem_trans->s, our_value);
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << " snoop value(2): " << &std::hex << our_value ) );
          }
          break;
          case 4: {
            uint32_t our_value = value_according_to_sb.getWord(mem_trans->s.physical_address & 7);
            SIM_set_mem_op_value_cpu(&mem_trans->s, our_value);
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << " snoop value(4): " << &std::hex << our_value ) );
          }
          break;
          case 8: {
            uint64_t our_value = value_according_to_sb.getDoubleWord(mem_trans->s.physical_address & 7);
            SIM_set_mem_op_value_cpu(&mem_trans->s, our_value);
            DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex) ( << " snoop value(8): " << &std::hex << our_value ) );
          }
          break;
          default:
            DBG_Assert( false, SetNumeric( (FlexusIdx) theIndex) ( << "Unsupported memory transaction size: " << mem_trans->s.size) );
        }

      }
    }

    return 0;
  }
};  // class SimicsTracerImpl

class SimicsTracer : public Simics::AddInObject <SimicsTracerImpl> {
  typedef Simics::AddInObject<SimicsTracerImpl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  static std::string className() {
    return "InOrderFeeder";
  }
  static std::string classDescription() {
    return "Flexus's In-order instruction feeder.";
  }

  SimicsTracer() : base() { }
  SimicsTracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  SimicsTracer(SimicsTracerImpl * anImpl) : base(anImpl) {}
};

}
