#define FLEXUS_BEGIN_COMPONENT NetworkTest
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <time.h>
#include <boost/random.hpp>

#define DBG_DefineCategories NetworkTest, Test
#define DBG_SetDefaultOps AddCat(NetworkTest | Test)
#include DBG_Control()

namespace nNetworkTest {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::intrusive_ptr;
using boost::minstd_rand;

#define NUM_NODES HACK_WIDTH
#define NUM_VC 3

typedef int64_t SumType;
typedef int64_t * SumType_p;

/*
FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(NetworkTestConfiguration,
    FLEXUS_PARAMETER( NumCycles, long, "Number of cycles", "nc", 1000 )
    FLEXUS_PARAMETER( NumMessages, long, "Number of messages per cycle", "nm", 3 )
);
*/

ParameterDefinition NumCycles_def_
(  "NumCycles"
   ,  "Number of cycles"
   ,  "nc"
);
typedef Parameter
< NumCycles_def_
, long
> NumCycles_param;
ParameterDefinition NumMessages_def_
(  "NumMessages"
   ,  "Number of messages per cycle"
   ,  "nm"
);
typedef Parameter
< NumMessages_def_
, long
> NumMessages_param;

struct NetCfgStruct {
  std::string theConfigName_;
  int64_t NumCycles;
  int64_t NumMessages;
}

template < class Param0 = use_default, class Param1 = use_default >
struct NetworkTestConfiguration {
  NetCfgStruct theCfg;

  typedef mpl::list <
  std::pair< typename Param0::tag, Param0 >,
      std::pair< typename Param1::tag, Param1 >
      > ArgList;

  typedef NetworkTestConfiguration<Param0, Param1> type;
  std::string name() {
    return theConfigName_;
  }
  NetworkTestConfiguration(const std::string & aConfigName) :
    theConfigName_(aConfigName)
    , NumCycles(theCfg, "1000")
    , NumMessages(theCfg, "3")
  {}

  typedef typename resolve < ArgList, NumCycles_param, NumCycles_param:: Static<1000> ::selected >::type NumCycles_t;
  NumCycles_t NumCycles;
  typedef typename resolve < ArgList, NumMessages_param, NumMessages_param:: Static<3> ::selected >::type NumMessages_t;
  NumMessages_t NumMessages;
};

template <class Configuration>
class NetworkTestComponent : public FlexusComponentBase< NetworkTestComponent, Configuration> {
  typedef FlexusComponentBase<nNetworkTest::NetworkTestComponent, Configuration> base;
  typedef typename base::cfg_t cfg_t;
  static std::string componentType() {
    return "nNetworkTest::NetworkTestComponent" ;
  }
public:
  using base::flexusIndex;
  using base::flexusWidth;
  using base::name;
  using base::statName;
  using base::cfg;
private:
  typedef typename base::self self;

public:
  NetworkTestComponent( cfg_t & aCfg, index_t anIndex, index_t aWidth )
    : base( aCfg, anIndex, aWidth, Flexus::Dbg::tSeverity::Severity(FLEXUS_internal_COMP_DEBUG_SEV) )
  {}

  // Initialization
  void initialize() {
    int32_t ii, jj;

    currCycle = 0;
    baseRand.seed(time(0));
    randNode = new boost::uniform_smallint<int> (0, NUM_NODES - 1);
    randVc = new  boost::uniform_smallint<int> ( 0, NUM_VC - 1);
    randValue = new boost::uniform_int<int> (0, 1 << 30);

    sendSums = new SumType_p[NUM_NODES];
    recvSums = new SumType_p[NUM_NODES];
    for (ii = 0; ii < NUM_NODES; ii++) {
      sendSums[ii] = new SumType[NUM_VC];
      recvSums[ii] = new SumType[NUM_VC];
      for (jj = 0; jj < NUM_VC; jj++) {
        sendSums[ii][jj] = 0;
        recvSums[ii][jj] = 0;
      }
    }
  }

  ~NetworkTestComponent() {
    int32_t ii, jj;
    for (ii = 0; ii < NUM_NODES; ii++) {
      for (jj = 0; jj < NUM_VC; jj++) {
        if (sendSums[ii][jj] != recvSums[ii][jj]) {
          std::cout << "sums didn't match\n";
        }
      }
    }
    std::cout << "done comparing sums\n";
  }

  // Ports
  struct ToNics : public PushOutputPortArray<NetworkTransport, NUM_NODES> { };

  struct FromNics : public PushInputPortArray<NetworkTransport, NUM_NODES>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aNetTest, index_t anIndex, NetworkTransport transport) {
      aNetTest.countRecv(transport, int(anIndex));
    }
  };

  //Drive Interfaces
  struct NetTestDrive {
    FLEXUS_DRIVE( NetTestDrive ) ;

    typedef FLEXUS_IO_LIST(1, Availability<ToNics> ) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNics> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aNetTest) {
      if (aNetTest.currCycle < aNetTest.cfg.NumCycles.value) {
        aNetTest.sendMsgs<FLEXUS_PASS_WIRING> ();
      }
      (aNetTest.currCycle)++;
    }
  };

  //Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, NetTestDrive) DriveInterfaces;

private:

  void countRecv(NetworkTransport & transport, int32_t dest) {
    intrusive_ptr<NetworkMessage> msg = transport[NetworkMessageTag];
    recvSums[dest][msg->vc] += transport[ProtocolMessageTag]->value;
  }

  FLEXUS_WIRING_TEMPLATE
  void sendMsgs() {
    // send the request number of messages, randomly distributed
    // (sources, destinations, and virtual channels)
    int32_t ii;
    for (ii = 0; ii < cfg.NumMessages.value; ii++) {
      int32_t src = (*randNode)(baseRand);
      int32_t dest = (*randNode)(baseRand);
      if (src != dest) {
        intrusive_ptr<NetworkMessage> msg(new NetworkMessage());
        msg->src = src;
        msg->dest = dest;
        msg->vc = (*randVc)(baseRand);

        intrusive_ptr<ProtocolMessage> val(new ProtocolMessage());
        val->value = (*randValue)(baseRand);

        NetworkTransport transport;
        transport.set(NetworkMessageTag, msg);
        transport.set(ProtocolMessageTag, val);
        FLEXUS_CHANNEL_ARRAY(*this, ToNics, src) << transport;

        sendSums[dest][msg->vc] += val->value;
      }
    }
  }

  // Tallies of cumulative sends and receives for each node and virtual channel
  SumType ** sendSums;
  SumType ** recvSums;

  // Random generators
  minstd_rand baseRand;
  boost::uniform_smallint<int> *randNode;
  boost::uniform_smallint<int> *randVc;
  boost::uniform_int<int> *randValue;

  // Current cycle number
  int64_t currCycle;

};

} // namespace nNetworkTest

#undef NUM_NODES
#undef NUM_VC

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NetworkTest

#define DBG_Reset
#include DBG_Control()
