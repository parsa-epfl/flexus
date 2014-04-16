#include <iostream>
#include <iomanip>
#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "SemanticInstruction.hpp"
#include "Effects.hpp"
#include "Constraints.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using nuArch::SemanticAction;
using nuArch::eResourceStatus;
using nuArch::kReady;

using nuArch::kSC;
using nuArch::kTSO;

bool checkStoreQueueAvailable( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  if ( anInstruction->core()->sbFull()) {
    return false;
  }
  return true;
}

boost::function<bool()> storeQueueAvailableConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkStoreQueueAvailable, anInstruction );
}

bool checkMembarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStSt();
}

boost::function<bool()> membarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarStoreStoreConstraint, anInstruction );
}

bool checkMembarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStLd();
}

boost::function<bool()> membarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarStoreLoadConstraint, anInstruction );
}

bool checkMembarSyncConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARSync();
}

boost::function<bool()> membarSyncConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarSyncConstraint, anInstruction );
}

bool checkMemoryConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  switch (anInstruction->core()->consistencyModel() ) {
    case kSC:
      if (! anInstruction->core()->speculativeConsistency()) {
        //Under nonspeculative SC, a load instruction may only retire when no stores are outstanding.
        if ( ! anInstruction->core()->sbEmpty()) {
          return false;
        }
      }
      break;
    case kTSO:
    case kRMO:
      //Under TSO and RMO, a load may always retire when it reaches the
      //head of the re-order buffer.
      break;
    default:
      DBG_Assert( false, ( << "Load Memory Instruction does not support consistency model " << anInstruction->core()->consistencyModel() ) );
  }
  return true;
}

boost::function<bool()> loadMemoryConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMemoryConstraint, anInstruction );
}

bool checkStoreQueueEmpty( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->sbEmpty();
}

boost::function<bool()> storeQueueEmptyConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkStoreQueueEmpty, anInstruction );
}

bool checkSideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->checkStoreRetirement(boost::intrusive_ptr<nuArch::Instruction>(anInstruction));
}

boost::function<bool()> sideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkSideEffectStoreConstraint, anInstruction );
}

} //nv9Decoder
