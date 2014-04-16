#include <memory>
#include <deque>

#define FLEXUS_BEGIN_COMPONENT TrussBWFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories TrussBWFetch, Fetch
#define DBG_SetDefaultOps AddCat(TrussBWFetch | Fetch)
#include DBG_Control()

namespace nTrussBWFetch {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

template <class Cfg>
class TrussBWFetchComponent : public FlexusComponentBase< TrussBWFetchComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(TrussBWFetchComponent, Cfg);

public:
  TrussBWFetchComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  void initialize() { }

  //Ports
public:
  //The FetchIn port is used to read InstructionTransports from the feeder object.  Note that
  //only the ArchitecturalInstruction slice of the InstructionTransport will be valid after
  //the instruction is read from the Feeder.
  struct FetchIn : public PullInputPort< InstructionTransport > { };

  //TrussBWFetch sends the instructions it fetches to the FetchOut port.
  struct FetchOut : public PushOutputPort< InstructionTransport > { };

  struct FromMembrane : public PushInputPort< NetworkTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aFetch, NetworkTransport aMessage) {
      // we're receiving a timestamp message

      // this better be a shadow node
      Flexus::SharedTypes::node_id_t theNodeId = aFetch.flexusIndex();
      boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(theNodeId);
      DBG_Assert(theTrussManager->isShadowNode(theNodeId));

      aFetch.tstampQueue.push_back(aMessage[TrussMessageTag]);

    }
  };

  //Drive Interfaces
public:
  //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
  //passing each instruction to its FetchOut port.
  struct FetchDrive {
    typedef FLEXUS_IO_LIST ( 3, Availability< FetchIn >, Availability<FetchOut> ) Inputs;
    typedef FLEXUS_IO_LIST ( 1, Value< FetchOut > ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & theComponent) {
      //Implementation is in the doFetch() member below

      Flexus::SharedTypes::node_id_t theNodeId = theComponent.flexusIndex();
      boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(theNodeId);

      if (theTrussManager->isPrimaryNode(theNodeId)) {
        //DBG_(Crit, (<< "Fetching on node: " << theNodeId << "<primary> cycle: " << theTrussManager->getCycleCount(theNodeId)));
        theComponent.doFetch<FLEXUS_PASS_WIRING>();
        theTrussManager->incrCycleCount(theNodeId);
      } else {
        if (theComponent.tstampQueue.empty()) {
          return;
        }

        boost::intrusive_ptr<nTrussMembrane::TrussMessage> tstamp = theComponent.tstampQueue.front();
        DBG_Assert(tstamp->type == nTrussMembrane::Timestamp, ( << "Non-timestamp message in BWFetch tstampQueue"));
        DBG_Assert(theTrussManager->getCycleCount(theNodeId) <= tstamp->cycle_count, ( << "timestamp slipped"));

        //DBG_(Crit, (<< "Fetching on node: " << theNodeId << "<shadow> cycle: " << theTrussManager->getCycleCount(theNodeId)));
        theComponent.doFetch<FLEXUS_PASS_WIRING>();

        while (!theComponent.tstampQueue.empty() && theTrussManager->getCycleCount(theNodeId) == tstamp->cycle_count) {
          // pop all timestamps from the queue that match this cycle count
          theComponent.tstampQueue.pop_front();
          if (!theComponent.tstampQueue.empty()) tstamp = theComponent.tstampQueue.front();
        }

        theTrussManager->incrCycleCount(theNodeId);
      }
    }
  };

  //Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST( 1, FetchDrive ) DriveInterfaces;

private:
  //Implementation of the FetchDrive drive interface
  FLEXUS_WIRING_TEMPLATE void doFetch() {
    for (int32_t i = 0; i < cfg.FetchBW.value; ++i) {
      InstructionTransport anInsn;

      //If the Feeder can accept a command and the FetchOut port can accept another
      //instruction
      if ( FLEXUS_CHANNEL( *this, FetchOut).available() ) {
        //If the Feeder is able to supply the requested instruction.
        if ( FLEXUS_CHANNEL( *this, FetchIn).available() ) {

          //Fetch it from the feeder
          FLEXUS_CHANNEL(*this, FetchIn) >> anInsn;

          DBG_(Iface, ( << "Fetched: " << *(anInsn[ArchitecturalInstructionTag])) Comp(*this) );

          //Send it to FetchOut
          FLEXUS_CHANNEL(*this, FetchOut) << anInsn;

          //Indicate that we made forward progress
          Flexus::Core::theFlexus->watchdogReset(flexusIndex());

        } else {
          DBG_( VVerb, ( << "FetchOut or FetchIn channel not available") Comp(*this)) ;
        }
      }
    } // end for BW
  } // end doFetch

  std::deque< boost::intrusive_ptr<TrussMessage> > tstampQueue;  // queue of TRUSS timestamps
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussBWFetchComponentConfiguration,
                                        FLEXUS_PARAMETER( FetchBW, int, "Fetch bandwidth", "fbw", 1 )
                                       );

}//End namespace nTrussBWFetch

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussBWFetch

#define DBG_Reset
#include DBG_Control()
