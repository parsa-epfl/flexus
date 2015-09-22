#include <components/v9Decoder/v9Decoder.hpp>
#include <components/v9Decoder/SemanticInstruction.hpp>

#define FLEXUS_BEGIN_COMPONENT v9Decoder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

#include <components/MTManager/MTManager.hpp>

namespace ll = boost::lambda;

namespace nv9Decoder {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

std::pair< boost::intrusive_ptr<AbstractInstruction>, bool> decode( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop );

class FLEXUS_COMPONENT(v9Decoder) {
  FLEXUS_COMPONENT_IMPL(v9Decoder);

  int64_t theInsnSequenceNo;
  std::list< boost::intrusive_ptr< AbstractInstruction > > theFIQ;

  bool theSyncInsnInProgress ;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(v9Decoder)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    //the FAG is always quiesced.
    return theFIQ.empty() && !theSyncInsnInProgress;
  }

  void initialize() {
    theInsnSequenceNo = 0;
    theSyncInsnInProgress = false;
  }

  void finalize() {}

public:

  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchBundleIn);
  void push(interface::FetchBundleIn const &, pFetchBundle & aBundle) {
    std::list< FetchedOpcode >::iterator iter = aBundle->theOpcodes.begin();
    std::list< FetchedOpcode >::iterator end = aBundle->theOpcodes.end();

    std::list< tFillLevel >::iterator fill_iter = aBundle->theFillLevels.begin();

    while (iter != end) {
      int32_t uop = 0;
      boost::intrusive_ptr<AbstractInstruction> insn;
      bool final_uop = false;
      //Note that multi-uop instructions can cause theFIQ to fill beyond its
      //configured size.
      while (! final_uop ) {
        std::tie( insn, final_uop ) = decode( *iter, flexusIndex(), ++theInsnSequenceNo, uop ++);
        insn->setFetchTransactionTracker(iter->theTransaction);

        // Set Fill Level for the insn
        insn->setSourceLevel(*fill_iter);

        theFIQ.push_back(insn);
      }
      ++iter;
      ++fill_iter;
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableFIQOut);
  int32_t pull( interface::AvailableFIQOut const &) {
    int32_t avail = cfg.FIQSize - theFIQ.size();
    if (avail < 0) {
      avail = 0;
    }
    return avail;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &) {
    return theFIQ.size() ;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &) {
    int32_t available_dispatch = 0;
    bool is_sync = false;
    std::pair<int, bool> dispatch_state;
    FLEXUS_CHANNEL(  AvailableDispatchIn ) >> dispatch_state;
    std::tie(available_dispatch, is_sync) = dispatch_state;

    return ( theFIQ.empty() || (available_dispatch == 0) || (theSyncInsnInProgress && !is_sync) );
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SquashIn);
  void push(interface::SquashIn const &, eSquashCause & aReason) {
    DBG_( Verb, Comp(*this) ( << "Dispatch SQUASH: " << aReason << " FIQ discarding: " << theFIQ.size() << " instructions" ));
    theFIQ.clear();

    FLEXUS_CHANNEL( SquashOut ) << aReason;
  }

public:
  void drive( interface::DecoderDrive const &) {
    //Implementation is in the doFetch() member below
    if (cfg.Multithread) {
      if (nMTManager::MTManager::get()->runThisD(flexusIndex())) {
        doDecode();
      }
    } else {
      doDecode();
    }
  }

private:
  //Implementation of the FetchDrive drive interface
  void doDecode() {

    DBG_Assert( FLEXUS_CHANNEL( AvailableDispatchIn ).available() ) ;
    //the FLEXUS_CHANNEL can only write to an lvalue, hence this rather
    //ugly construction.
    int32_t available_dispatch = 0;
    bool is_sync = false;
    std::pair<int, bool> dispatch_state;
    FLEXUS_CHANNEL(  AvailableDispatchIn ) >> dispatch_state;
    std::tie(available_dispatch, is_sync) = dispatch_state;

    int64_t theOpcode;

    if (is_sync) {
      //If we had a synchronized instruction in progress, it has completed
      theSyncInsnInProgress = false;
    }
    uint32_t dispatched = 0;
    while (    available_dispatch > 0
               && dispatched < cfg.DispatchWidth
               && !theFIQ.empty()
               && !theSyncInsnInProgress
          ) {
      if (theFIQ.front()->haltDispatch()) {
        //May only dispatch black box op when core is synchronized
        if (is_sync && dispatched == 0) {
          DBG_( Verb, Comp(*this) ( << "Dispatch: (halt-dispatch) " << *theFIQ.front()));
          boost::intrusive_ptr< AbstractInstruction > inst(theFIQ.front());
          theFIQ.pop_front();

          // Report the dispatched instruction to the PowerTracker
          theOpcode = (dynamic_cast< v9Instruction *>( inst.get() ))->getOpcode();
          FLEXUS_CHANNEL( DispatchedInstructionOut ) << theOpcode;

          FLEXUS_CHANNEL( DispatchOut ) << inst;
          theSyncInsnInProgress = true;
        } else {
          break; //No more dispatching this cycle
        }

      } else {
        DBG_( Verb, Comp(*this) ( << "Dispatch: " << *theFIQ.front()));
        boost::intrusive_ptr< AbstractInstruction > inst(theFIQ.front());
        theFIQ.pop_front();

        // Report the dispatched instruction to the PowerTracker
        theOpcode = (dynamic_cast< v9Instruction *>( inst.get() ))->getOpcode();
        FLEXUS_CHANNEL( DispatchedInstructionOut) << theOpcode;

        FLEXUS_CHANNEL( DispatchOut ) << inst;
      }
      ++dispatched;
      --available_dispatch;
    }

  }
};

}//End namespace nv9Decoder

FLEXUS_COMPONENT_INSTANTIATOR( v9Decoder, nv9Decoder);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT v9Decoder

#define DBG_Reset
#include DBG_Control()
