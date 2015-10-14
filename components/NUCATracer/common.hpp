#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <map>
#include <list>
#include <vector>
#include <iostream>

#include <functional>
#include <boost/utility.hpp>
#include <core/debug/debug.hpp>
#include <core/stats.hpp>

#define TARGET_PLATFORM v9
#include <core/target.hpp>
#include <core/types.hpp>
#include <core/boost_extensions/lexical_cast.hpp>
#include <memory>

#include <components/Common/Slices/FillType.hpp>
#include <components/Common/Slices/FillLevel.hpp>

#include <boost/preprocessor/expand.hpp>
#define FDT_HELPER , 0
#define FDT_NOT_DEFINED_FDT_HELPER 1 , 1
#define FLEXUS_DEFINED_TEST(x) ( FDT_NOT_DEFINED_ ## x )
#define FLEXUS_GET_SECOND(x,y) y
#define FLEXUS_IS_EMPTY( x ) BOOST_PP_EXPAND( FLEXUS_GET_SECOND FLEXUS_DEFINED_TEST(x FDT_HELPER) )

#include <boost/preprocessor/control/iif.hpp>
#define DECLARE_STATIC_PARAM( x )   StatAnnotation * Cfg ## x ;

#define ANNOTATE_STATIC_PARAM( x )                                 \
  Cfg ## x = new StatAnnotation( "Configuration." # x );           \
  char * Cfg ## x ## str =                                         \
  BOOST_PP_IIF                                                     \
    ( FLEXUS_IS_EMPTY( x )                                         \
    , "true"                                                       \
    , "false"                                                      \
    ) ;                                                            \
  * Cfg ## x = Cfg ## x ## str;                                    \
  DBG_(Dev, ( << "Cfg: " # x ": " << Cfg ## x ## str ) );             /**/

template <int32_t w, typename Source>
std::string fill(Source s) {
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(w) << boost::lexical_cast<std::string>(s);
  return ss.str();
}

typedef uint64_t tTime;
typedef uint32_t tAddress;
typedef uint64_t tVAddress;
typedef int32_t tCoreId;
typedef uint64_t tThreadId;

enum tEventType {
  eRead,
  eWrite,
  eFetch,
  eL1CleanEvict,
  eL1DirtyEvict,
  eL1IEvict
};

using Flexus::SharedTypes::tFillType;
using Flexus::SharedTypes::eCold;
using Flexus::SharedTypes::eReplacement;
using Flexus::SharedTypes::eCoherence;
using Flexus::SharedTypes::eDMA;
using Flexus::SharedTypes::ePrefetch;
using Flexus::SharedTypes::tFillLevel;
using Flexus::SharedTypes::eLocalMem;

struct TraceCoordinator {
  virtual ~TraceCoordinator() {}

  virtual void accessL2(tTime aTime
                        , tEventType aType
                        , tCoreId aNode
                        , tThreadId aThread
                        , tAddress anAddress
                        , tVAddress aPC
                        , tFillType aFillType
                        , tFillLevel aFillLevel
                        , bool anOS)
  = 0;
  virtual void accessOC(tTime aTime
                        , tEventType aType
                        , tCoreId aNode
                        , tThreadId aThread
                        , tAddress anAddress
                        , tVAddress aPC
                        , tFillType aFillType
                        , tFillLevel aFillLevel
                        , bool anOS)
  = 0;
  virtual void finalize( void ) = 0;
  virtual void processTrace( void ) = 0;
};

struct TraceData {
  TraceData( uint64_t aTime
             , tEventType anEventType
             , tCoreId aNode
             , tThreadId aThread
             , tAddress anAddress
             , tVAddress aPC
             , tFillType aFillType
             , tFillLevel aFillLevel
             , bool isOS
           )
    : theTime(aTime)
    , theEventType(anEventType)
    , theNode(aNode)
    , theThread(aThread)
    , theAddress(anAddress)
    , thePC(aPC)
    , theFillType(aFillType)
    , theFillLevel(aFillLevel)
    , theOS(isOS)
  {}
  TraceData(void) { }

  tTime theTime;
  tEventType theEventType;
  tCoreId theNode;
  tThreadId theThread;
  tAddress theAddress;
  tVAddress thePC;
  tFillType theFillType;
  tFillLevel theFillLevel;
  bool theOS;
};

std::ostream & operator << (std::ostream & str, TraceData const & aTraceData);

TraceCoordinator * initialize_NUCATracer(int32_t aNumNodes);

#endif //COMMON_INCLUDED
