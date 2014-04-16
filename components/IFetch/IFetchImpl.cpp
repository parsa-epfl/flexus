#include <components/IFetch/IFetch.hpp>

#include <memory>

#include <components/Common/Slices/ExecuteState.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

#include <core/simics/simics_interface.hpp>

#define FLEXUS_BEGIN_COMPONENT IFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories IFetch
#define DBG_SetDefaultOps AddCat(IFetch) Comp(*this)
#include DBG_Control()

namespace nIFetch {

using namespace Flexus;
using namespace Flexus::Simics;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

using boost::intrusive_ptr;

class FLEXUS_COMPONENT(IFetch) {
  FLEXUS_COMPONENT_IMPL( IFetch );

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(IFetch)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , statLineBufferHits(statName() + "-LineBufferHits") {
    theState = Idle;
  }

private:
  // the pending instruction
  InstructionTransport theInsn;
  // the current state
  enum FetchState {
    Idle,
    Pending_1,
    Pending_2,//an x86 Ifetch may span multiple cache lines (at most 2).
    Ready
  };
  FetchState theState;

  PhysicalMemoryAddress theLineBuffer;
  Stat::StatCounter statLineBufferHits;

public:
  void initialize() {
    theState = Idle;
    DBG_(Crit, ( << "StallInstructions: " << cfg.StallInstructions));
    theLineBuffer = PhysicalMemoryAddress(0);
  }

  void finalize () {}

  bool isQuiesced() const {
    return theState == Idle; //BWFetchComponent is always quiesced
  }

public:
  //IMemIn
  //------
  FLEXUS_PORT_ALWAYS_AVAILABLE(IMemIn);
  void push(interface::IMemIn const &, MemoryTransport & aMemTransport) {
    memoryResponse(aMemTransport);
  }

public:
  void drive( interface::FeederDrive const &) {
    DBG_(VVerb, ( << "IFetchDrive" ) );
    doFetch();
  }
  void drive( interface::PipelineDrive const &) {
    DBG_(VVerb, ( << "PipelineDrive" ));
    //Implementation is in the doPipe() member below
    doPipe();
  }

private:
  //Handles an incoming memory message from the Instruction Memory
  void memoryResponse(MemoryTransport transport) {

    // Handle coherence messages first
    if ( transport[MemoryMessageTag]->isRequest() ) {

      intrusive_ptr<MemoryMessage> req ( transport[MemoryMessageTag] );
      intrusive_ptr<MemoryMessage> reply;

      switch ( req->type() ) {
        case MemoryMessage::Invalidate:
          reply = new MemoryMessage(MemoryMessage::InvalidateAck, req->address());
          break;
        case MemoryMessage::Downgrade:
          reply = new MemoryMessage(MemoryMessage::DowngradeAck, req->address());
          break;
        case MemoryMessage::Probe:
          reply = new MemoryMessage(MemoryMessage::ProbedNotPresent, req->address());
          break;
        case MemoryMessage::ReturnReq:
          reply = new MemoryMessage(MemoryMessage::ReturnReply, req->address());
          break;
        default:
          DBG_( Crit, ( << "Received a request from memory that is not Invalidate, Downgrade, Return, or Probe: " << *req ) Addr(req->address()) );
          DBG_Assert( 0 );
      }

      DBG_Assert ( FLEXUS_CHANNEL ( IMemSnoopOut).available() );
      transport.set ( MemoryMessageTag, reply );
      DBG_ ( Verb, ( << "Sending memory reply: " << *reply ) Addr ( reply->address() ) );
      FLEXUS_CHANNEL ( IMemSnoopOut ) << transport;
      return;
    }

    PhysicalMemoryAddress ResponseAddress = theInsn[ArchitecturalInstructionTag]->physicalInstructionAddress();

    DBG_Assert(theState == Pending_1 || theState == Pending_2);

#if FLEXUS_TARGET_IS(v9)
    DBG_Assert(ResponseAddress == transport[MemoryMessageTag]->address());
    DBG_(Iface, ( << "received I-mem response: " << *(transport[MemoryMessageTag]) ));

    if (transport[TransactionTrackerTag]) {
      transport[TransactionTrackerTag]->complete();
    }

    theState = Ready;
#else
    DBG_Assert((transport[MemoryMessageTag]->address() == ResponseAddress) ||
               (transport[MemoryMessageTag]->address() == ((ResponseAddress | (cfg.ICacheLineSize - 1)) + 1))//assuming a 64 bytes cacheline
              );
    DBG_(VVerb, ( << "Response address = " << std::hex << (uint32_t)transport[MemoryMessageTag]->address() << " : "
                  << "waiting for " << (uint32_t)ResponseAddress << " or "
                  << (uint32_t)( (ResponseAddress | (cfg.ICacheLineSize - 1)) + 1) << " ."
                  << "Wait state : " << theState)
        );

    theState = theState == Pending_2 ? Pending_1 : Ready;

    if ( theState == Ready) {
      if (transport[TransactionTrackerTag]) {
        transport[TransactionTrackerTag]->complete();
      }
    }
#endif

  }

  //Implementation of the PipelineDrive drive interface
  void doPipe() {
    if (theState == Ready) {
      if (FLEXUS_CHANNEL(FetchOut).available()) {
        //Send it to FetchOut
        //DBG_(Iface, ( << "passing instruction along: "
        //                     << *(theInsn[ArchitecturalInstructionTag]) ));
        FLEXUS_CHANNEL(FetchOut) << theInsn;
        theState = Idle;
      } else {
        DBG_(VVerb, ( << "execute space not available, blocking ready instruction" ));
      }
    }
  }

  bool hitLineBuffer(PhysicalMemoryAddress const & anAddress) {
    if ( (anAddress & ~63) ==  theLineBuffer) {
      ++statLineBufferHits;
      return true;
    } else {
      theLineBuffer = PhysicalMemoryAddress(anAddress & ~63);
      return false;
    }
  }

  //Implementation of the FetchDrive drive interface
  void doFetch() {

    //#warning Locked to two processors for now.
    //      if ( flexusIndex() > 3 ) return;
    //Instruct the Feeder to give us the next instruction along the path that
    //will eventually commit.

    //If the Feeder is able to supply the requested instruction.
    if ( theState == Idle && FLEXUS_CHANNEL(FetchIn).available() ) {

      //Fetch it from the feeder
      FLEXUS_CHANNEL(FetchIn) >> theInsn;
      //DBG_(Iface, (<< "Fetched: " << *(theInsn[ArchitecturalInstructionTag]) ));

      theState = Ready;
#if FLEXUS_TARGET_IS(x86)
      if (theInsn[ArchitecturalInstructionTag]->getIfPart() != Normal) {
        //we received an instruction for which we must not simulate the I-fetch part.
        //Return and keep the 'theState' variable set to 'Ready'. The next time the doPipe()
        //function is called it will forward the instruction to the next pipeline stage
        return;
      }
#endif
      if (cfg.StallInstructions) {

        PhysicalMemoryAddress currentPC = theInsn[ArchitecturalInstructionTag]->physicalInstructionAddress();
#if FLEXUS_TARGET_IS(x86)
        //the size of an x86 instruction can be between 1 and ~16 bytes
        DBG_Assert((theInsn[ArchitecturalInstructionTag]->InstructionSize() > 0) &&
                   (theInsn[ArchitecturalInstructionTag]->InstructionSize() < 20)
                  );

        if (((currentPC & (cfg.ICacheLineSize - 1)) + theInsn[ArchitecturalInstructionTag]->InstructionSize()) > cfg.ICacheLineSize) {
          //instruction fetch spans to multiple cache blocks...
          PhysicalMemoryAddress consequtivePC = (PhysicalMemoryAddress)(((uint32_t)((theInsn[ArchitecturalInstructionTag]->physicalInstructionAddress()) | (cfg.ICacheLineSize - 1)) + 1));
          DBG_(VVerb, ( << "Instruction fetch spans to multiple cache blocks. Address " << std::hex << ((uint32_t)currentPC)));

          if ( hitLineBuffer(currentPC) ) {
            DBG_(VVerb, ( << "First Cache line request hits in the line buffer. Address " << std::hex << ((uint32_t)currentPC)));
          } else {
            DBG_(VVerb, ( << " First Cache line request issued. Address " << std::hex << ((uint32_t) currentPC)));
            theState = theState == Pending_1 ? Pending_2 : Pending_1;
            doMemRequest(currentPC);
          }

          if ( hitLineBuffer(consequtivePC)) {
            DBG_(VVerb, ( << "Second Cache line request hits in the line buffer. Address " << std::hex << ((uint32_t) consequtivePC)));
          } else {
            DBG_(VVerb, ( << " Second Cache line request issued. Address " << std::hex << ((uint32_t) consequtivePC)));
            theState = theState == Pending_1 ? Pending_2 : Pending_1;
            doMemRequest(consequtivePC);
          }
        } else {
          //instruction fetch lies within the boundaries of a single cache line
          if ( hitLineBuffer(currentPC)) {
            DBG_(VVerb, ( << "IFetch request hits in the line buffer. Address " << std::hex << ((uint32_t)currentPC)));
            theState = Ready;
          } else {
            //Send the appropriate request to the instruction memory
            theState = Pending_1;
            DBG_(VVerb, ( << "IFetch request issued. Address " << std::hex << ((uint32_t) currentPC)));
            doMemRequest(currentPC);
          }
        }
#else //v9
        if ( hitLineBuffer(currentPC)) {
          theState = Ready;
        } else {
          //Send the appropriate request to the instruction memory
          theState = Pending_1;
          doMemRequest(currentPC);
        }
#endif
      } else {
        theState = Ready;
      }
    }
  }

  //Implementation of the FetchDrive drive interface
  void doMemRequest(PhysicalMemoryAddress thePC) {
    FLEXUS_PROFILE();
    // the channel is guaranteed to be available (see above)
    intrusive_ptr<ArchitecturalInstruction> inst( theInsn[ArchitecturalInstructionTag] );

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(thePC);
    tracker->setInitiator(flexusIndex());
    tracker->setSource("IFetch");
    tracker->setCriticalPath(true);
    tracker->setFetch(true);
    if (inst->isPriv()) {
      tracker->setOS(true);
    }
    tracker->setDelayCause(name(), "IFetch");

    // create new memory message - first arg is mem address to fetch, second is PC (both set to PC here)
    intrusive_ptr<MemoryMessage> msg( MemoryMessage::newFetch(thePC));
    MemoryTransport transport;
    transport.set(MemoryMessageTag, msg);
    transport.set(TransactionTrackerTag, tracker);

    DBG_(Iface, ( << "sending I-mem request: " << *msg ));
    FLEXUS_CHANNEL(IMemOut) << transport;
  }

};

}//End namespace IFetch

FLEXUS_COMPONENT_INSTANTIATOR( IFetch, nIFetch );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT IFetch

