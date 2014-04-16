#include "common.hpp"
#include "trace.hpp"
#include "coordinator.hpp"

#define DBG_DefineCategories ExperimentDbg, ReaderDbg, IgnoreDbg
#define DBG_SetInitialGlobalMinSev Iface
#include DBG_Control()

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;
#include <boost/utility.hpp>
#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

std::ostream & operator << (std::ostream & str, TraceData const & aTraceData) {
  str << std::setw(12) << aTraceData.theFillLevel ;
  str << "[" << fill<2>(aTraceData.theNode) << "]";
  str << " Thr 0x" << std::hex << fill<2>(aTraceData.theThread) << std::dec ;
  str << " PC " << std::setw(8) << std::hex << aTraceData.thePC << std::dec ;
  switch (aTraceData.theEventType) {
    case eRead:
      str << "  R @ ";
      break;
    case eWrite:
      str << "  W @ ";
      break;
    case eFetch:
      str << "  F @ ";
      break;
    case eL1CleanEvict:
      str << " CE @ ";
      break;
    case eL1DirtyEvict:
      str << " DE @ ";
      break;
    case eL1IEvict:
      str << " IE @ ";
      break;
  }
  str << std::setw(8) << std::hex << aTraceData.theAddress << std::dec ;

  return str;
}

tCoordinator::tCoordinator(std::string const & aPath, const bool aReadOnlyFlag ) {
  trace::initialize(aPath, aReadOnlyFlag);
}

tCoordinator::~tCoordinator() {}

void tCoordinator::accessL2(tTime aTime, tEventType aType, tCoreId aNode, tThreadId aThread, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  TraceData evt(aTime, aType, aNode, aThread, anAddress, aPC, aFillType, aFillLevel, anOS );
  trace::addL2Access(evt);
}

void tCoordinator::accessOC(tTime aTime, tEventType aType, tCoreId aNode, tThreadId aThread, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  TraceData evt(aTime, aType, aNode, aThread, anAddress, aPC, aFillType, aFillLevel, anOS );
  trace::addOCAccess(evt);
}

int32_t tCoordinator::getL2Access(TraceData * aRecord) {
  return trace::getL2Access(aRecord);
}

int32_t tCoordinator::getOCAccess(TraceData * aRecord) {
  return trace::getOCAccess(aRecord);
}

void tCoordinator::finalize() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, ( << "Finalizing state. " << boost::posix_time::to_simple_string(now)));

  trace::flushFiles();
}

void tCoordinator::processTrace( void ) {
}
