#define FLEXUS_BEGIN_COMPONENT DGP
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories DGP
#define DBG_SetDefaultOps AddCat(DGP)
#include DBG_Control()

#define theDGPInitialConf 1
#define theDGPThresholdConf 2
#define theDGPMaxConf 3

#include "DgpMain.hpp"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace nDGP {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::intrusive_ptr;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;
typedef nDgpTable::DgpMain DgpMain;

template <class Cfg>
class DGPComponent : public FlexusComponentBase<DGPComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(nDGP::DGPComponent, Cfg);

  // the actual DGP object
  std::unique_ptr<DgpMain> theDgp;

public:
  std::list<MemoryTransport> toCache_Snoop;

  DGPComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theDgp()
  {}

  ~DGPComponent() { }

  bool isQuiesced() const {
    return toCache_Snoop.empty();
  }

  void saveState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << *theDgp;

    // close archive
    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);

    // BTG HACK to use only lower (0-15) dgp state
    char name[32];
    sprintf(name, "%03d-dgp", flexusIndex() % 16);
    DBG_(Crit, ( << "Serializing DGP from " << name));

    fname += "/" + std::string(name);
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (ifs.is_open()) {
      boost::archive::binary_iarchive ia(ifs);

      ia >> *theDgp;

      // close archive
      ifs.close();
    } else {
      DBG_(Crit, ( << "DGP state file " << fname << " not found"));
    }

    if (cfg.DgpSigTableFile.value != "theSigTableFile") {
      DBG_(Crit, ( << "Loading DGP SigTable from " << cfg.DgpSigTableFile.value));

      std::string othername(cfg.DgpSigTableFile.value);
      othername += "/" + std::string(name);
      std::ifstream otherifs(othername.c_str(), std::ios::binary);
      DBG_Assert(otherifs.is_open(), ( << "Couldn't open " << othername));

      DBG_(Crit, ( << "Opening archive for " << othername));
      boost::archive::binary_iarchive oia(otherifs);

      DgpMain theOtherDgp(cfg.BlockAddrBits.value,
                          cfg.PCBits.value,
                          cfg.L2BlockSize.value,
                          cfg.NumSets.value,
                          cfg.Assoc.value);
      DBG_(Crit, ( << "Loading DGP state from " << othername));
      oia >> theOtherDgp;
      DBG_(Crit, ( << "Writing SigTable state"));
      theOtherDgp.writeSigTable("tempsigtable");
      DBG_(Crit, ( << "Reading SigTable state"));
      theDgp->readSigTable("tempsigtable");
      DBG_(Crit, ( << "Done with " << othername));

    }

  }

  // Initialization
  void initialize() {
    theDgp.reset(
      new DgpMain
      ( statName()
        , flexusIndex()
        , cfg.BlockAddrBits.value
        , cfg.PCBits.value
        , cfg.L2BlockSize.value
        , cfg.NumSets.value
        , cfg.Assoc.value
      )
    );
  }

  // Ports
  struct ToCpu : public PushOutputPort< MemoryTransport > { };
  struct ToL1_Request : public PushOutputPort< MemoryTransport > { };
  struct ToL1_Snoop : public PushOutputPort< MemoryTransport > { };

  struct FromL1 : public PushInputPort<MemoryTransport>, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aDgp, MemoryTransport aMessageTransport) {
      MemoryAddress blockAddr = aDgp.blockAddress(aMessageTransport[MemoryMessageTag]->address());

      DBG_(Iface, Comp(aDgp) Addr(blockAddr)
           ( << "message received from L1: " << *aMessageTransport[MemoryMessageTag] ) );

      switch (aMessageTransport[MemoryMessageTag]->type()) {
        case MemoryMessage::Downgrade:
          //DBG_(Crit, Comp(aDgp) (<< "Got a downgrade for " << blockAddr));
          aDgp.recvSnoop(blockAddr, true);
          break;
        case MemoryMessage::Invalidate:
          //DBG_(Crit, Comp(aDgp) (<< "Got an invalidation for " << blockAddr));
          aDgp.recvSnoop(blockAddr, false);
          break;
        case MemoryMessage::Probe:
        case MemoryMessage::LoadReply:
          // do nothing
          break;
        case MemoryMessage::StorePrefetchReply: {
          bool anyInvs = aMessageTransport[MemoryMessageTag]->anyInvs();
          if (anyInvs) {
            //DBG_(Crit, Comp(aDgp) (<< "Got StorePrefetchReply with anyInvs set for " << blockAddr));
            aDgp.recvSnoop(blockAddr, false);
          }
          break;
        }
        case MemoryMessage::StoreReply:
        case MemoryMessage::CmpxReply:
        case MemoryMessage::RMWReply: {
          //DBG_Assert(aMessageTransport[ExecuteStateTag]);
          //nExecute::ExecuteState & ex = static_cast<nExecute::ExecuteState &>(*aMessageTransport[ExecuteStateTag]);
          //MemoryAddress pc = ex.instruction().physicalInstructionAddress();
          MemoryAddress pc = aMessageTransport[MemoryMessageTag]->pc();
          bool anyInvs = aMessageTransport[MemoryMessageTag]->anyInvs();
          //DBG_(Crit, Comp(aDgp) (<< "Got a StoreReply for " << blockAddr << (anyInvs ? " with invs" : " no invs")));
          //if (anyInvs) {
          //  DBG_(Crit, Comp(aDgp) (<< "Got StoreReply with anyInvs set for " << blockAddr));
          //}
          bool isOS = false;
          DBG_Assert(aMessageTransport[TransactionTrackerTag],
                     ( << "no Tracker in DGP for: " << *aMessageTransport[MemoryMessageTag] ) );
          //DBG_Assert(aMessageTransport[TransactionTrackerTag]->OS(),
          //           ( << "no OS state in DGP for: " << *aMessageTransport[MemoryMessageTag] ) );
          if (aMessageTransport[TransactionTrackerTag]->OS()) {
            isOS = *aMessageTransport[TransactionTrackerTag]->OS();
          }
          if (aDgp.recvWrite(blockAddr, pc, anyInvs, isOS)) {
            // there was a misprediction
            if (aMessageTransport[TransactionTrackerTag]) {
              if (aMessageTransport[TransactionTrackerTag]->fillType()) {
                aMessageTransport[TransactionTrackerTag]->setFillType(nTransactionTracker::eDGP);
              }
            }
          }
        }
        break;
        default:
          DBG_(Crit, Comp(aDgp)
               Addr(blockAddr)
               ( << "unexpected message: " << *aMessageTransport[MemoryMessageTag] ) );
          DBG_Assert(false, Comp(aDgp));
      }

      DBG_(VVerb,  Comp(aDgp)
           Addr(blockAddr)
           ( << "message queued for cpu: " << *aMessageTransport[MemoryMessageTag] ) );
      FLEXUS_CHANNEL(aDgp, ToCpu) << aMessageTransport;
    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aDgp) {
      return FLEXUS_CHANNEL(aDgp, ToCpu).available() && (aDgp.toCache_Snoop.size() < aDgp.cfg.SnoopQueue.value);
    }
  };

  // A port that produces a MemoryMessage with type FlushReq for a downgrade request
  struct FromCPU_Request : public PushInputPort<MemoryTransport>, AvailabilityComputedOnRequest {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aDgp, MemoryTransport aMessageTransport) {
      DBG_(Iface, Comp(aDgp)
           Addr(aDgp.blockAddress(aMessageTransport[MemoryMessageTag]->address()))
           ( << "message received from CPU: " << *aMessageTransport[MemoryMessageTag] ) );

      //if (aDgp.blockAddress(aMessageTransport[MemoryMessageTag]->address()) == 0x00305a9c0) {
      //  DBG_(Crit, Comp(aDgp) ( << "message received from CPU: " << *aMessageTransport[MemoryMessageTag] ) );
      //}

      FLEXUS_CHANNEL( aDgp, ToL1_Request) << aMessageTransport;
    }
    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aDgp) {
      return FLEXUS_CHANNEL(aDgp, ToL1_Request).available();
    }
  };

  struct FromCPU_Snoop: public PushInputPort<MemoryTransport>, AvailabilityComputedOnRequest {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aDgp, MemoryTransport aMessageTransport) {
      DBG_(Iface, Comp(aDgp)
           Addr(aDgp.blockAddress(aMessageTransport[MemoryMessageTag]->address()))
           ( << "message received from CPU: " << *aMessageTransport[MemoryMessageTag] ) );

      //if (aDgp.blockAddress(aMessageTransport[MemoryMessageTag]->address()) == 0x00305a9c0) {
      //  DBG_(Crit, Comp(aDgp) ( << "message received from CPU: " << *aMessageTransport[MemoryMessageTag] ) );
      //}

      aDgp.toCache_Snoop.push_back(aMessageTransport);
    }
    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aDgp) {
      return aDgp.toCache_Snoop.size() < aDgp.cfg.SnoopQueue.value;
    }

  };

  struct DGP_TowardsCaches_Drive {
    FLEXUS_DRIVE( DGP_TowardsCaches_Drive ) ;
    typedef FLEXUS_IO_LIST(2, Availability<ToL1_Snoop>
                           , Availability<ToL1_Request>
                           , Value<FromCPU_Snoop>
                           , Value<FromCPU_Request>
                          ) Inputs;
    typedef FLEXUS_IO_LIST(2, Value<ToL1_Snoop>, Value<ToL1_Request>
                          ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aDGP) {
      DBG_(VVerb, Comp(aDGP) ( << "DGP_TowardsCaches_Drive" ) ) ;
      while ( ! aDGP.toCache_Snoop.empty() && FLEXUS_CHANNEL(aDGP, ToL1_Snoop).available() ) {
        DBG_(Iface, Comp(aDGP)
             Addr(aDGP.blockAddress(aDGP.toCache_Snoop.front()[MemoryMessageTag]->address()))
             ( << "message sent to caches: " << *aDGP.toCache_Snoop.front()[MemoryMessageTag] ) );

        FLEXUS_CHANNEL(aDGP, ToL1_Snoop) << aDGP.toCache_Snoop.front();
        aDGP.toCache_Snoop.pop_front();
      }
    }
  };

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST (1, DGP_TowardsCaches_Drive) DriveInterfaces;

private:

  bool recvWrite(MemoryAddress addr, MemoryAddress pc, bool anyInvs, bool isOS) {
    // first snoop the DGP if necessary
    if (anyInvs) {
      DBG_(VVerb, Comp(*this) ( << "snooping on write for " << std::hex << addr ) );
      theDgp->snoop(addr);
    }

    // tell DGP about the write
    DBG_(VVerb, Comp(*this) ( << "got write for " << std::hex << addr ) );
    if (theDgp->write(addr, pc, isOS)) {
      if (cfg.Downgrade.value) {

        MemoryTransport transport;
        intrusive_ptr<MemoryMessage> msg( new MemoryMessage(MemoryMessage::FlushReq, addr) );

        boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
        tracker->setAddress(addr);
        tracker->setInitiator(flexusIndex());
        tracker->setSource(name() + " Downgrade");
        tracker->setDelayCause(name(), "Downgrade");
        tracker->setOS(isOS);

        transport.set(MemoryMessageTag, msg);
        transport.set(TransactionTrackerTag, tracker);
        DBG_(Iface, Comp(*this) Addr(addr)
             ( << "downgrade predicted: " << *msg ) );
        toCache_Snoop.push_back(transport);
      }
    }
    if (cfg.Downgrade.value) {
      return theDgp->wasMispredict();
    }
    return false;
  }

  void recvSnoop(MemoryAddress addr, bool isDowngrade) {
    DBG_(VVerb, Comp(*this) Addr(addr)
         ( << "got " << (isDowngrade ? "downgrade" : "invalidate") << " for " << std::hex << addr ) );
    theDgp->snoop(addr);
  }

  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(cfg.L2BlockSize.value - 1) );
  }

};

std::string theSigTableFile("theSigTableFile");

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(DGPConfiguration,
                                        FLEXUS_PARAMETER( Downgrade, bool, "Perform downgrades?", "downgrade", true )
                                        FLEXUS_PARAMETER( NumSets, int, "# Sets in sig table", "sets", 128 )
                                        FLEXUS_PARAMETER( Assoc, int, "Assoc of sig table", "assoc", 4)
                                        FLEXUS_PARAMETER( BlockAddrBits, int, "Addr bits in sig", "addr_bits", 12)
                                        FLEXUS_PARAMETER( PCBits, int, "PC bits used in history", "pc_bits", 15 )
                                        FLEXUS_PARAMETER( L2BlockSize, int, "Block size of L2 cache", "bsize", 64 )
                                        FLEXUS_PARAMETER( SnoopQueue, uint32_t, "Size of DGP Snoop/Downgrade Queue", "squeue", 16 )
                                        FLEXUS_PARAMETER( DgpSigTableFile, std::string, "File to serialize SigTable from", "sigfile", theSigTableFile )
                                       );

} //End Namespace nDGP

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DGPComponent

#define DBG_Reset
#include DBG_Control()

