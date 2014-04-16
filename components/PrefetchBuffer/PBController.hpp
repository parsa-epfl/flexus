/*! \file PBController.hpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/PrefetchMessage.hpp>

namespace nPrefetchBuffer {

enum eAction {
  kNoAction
  , kSendToFront
  , kSendToFrontAndClearMAF
  , kSendToBackSnoop
  , kSendToBackRequest
  , kBlockOnAddress
  , kBlockOnPrefetch
  , kBlockOnWatch
  , kRemoveAndWakeMAF
};

enum eProbeMAFResult {
  kNoMatch
  , kCancelled
  , kNotCancelled
  , kWatch
};

using namespace Flexus::SharedTypes;
typedef PhysicalMemoryAddress MemoryAddress;

struct PB {
  virtual void sendMaster( boost::intrusive_ptr<PrefetchMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
  virtual void sendBackRequest( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
  virtual void sendBackPrefetch( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
  virtual void sendBackSnoop( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
  virtual void sendFront( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
  virtual bool cancel( MemoryAddress const & anAddress ) = 0;
  virtual eProbeMAFResult probeMAF( MemoryAddress const & anAddress ) = 0;
  virtual ~PB() {}
};

struct PBController {
  static PBController * construct(PB * aPB, std::string const & aName, unsigned aNodeId, int32_t aNumEntries, int32_t aWatchEntries, bool anEvictClean, bool aUseStreamFetch) ;
  virtual eAction processRequestMessage( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker, bool hasMAFConflict, bool waited ) = 0;
  //Possible returns: kSendToFront, kSendToBackRequest, kBlockOnAddress
  virtual eAction processBackMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) = 0;
  //Possible returns: kSendToFront, kRemoveAndWakeMAF
  virtual eAction processSnoopMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) = 0;
  //Possible returns: kSendToBackSnoop, kRemoveAndWakeMAF, kNoAction
  virtual eAction processPrefetch( boost::intrusive_ptr<PrefetchMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker, bool hasMAFConflict ) = 0;
  //Possible returns: kBlockOnPrefetch, kNoAction
  virtual ~PBController() {}
  virtual void saveState(std::string const & aDirName) const = 0;
  virtual void loadState(std::string const & aDirName) = 0;
};

}  // end namespace nPrefetchBuffer

