#include <deque>
#include <memory>

#include <boost/random.hpp>
#include <boost/integer_traits.hpp>

#define FLEXUS_BEGIN_COMPONENT RandomFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace Flexus {
namespace RandomFeeder {

using boost::mt19937;
using boost::uniform_real;
using boost::uniform_int;
using boost::integer_traits;
using std::deque;
using std::auto_ptr;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

using boost::intrusive_ptr;

template <class RandomFeederComponentConfigurationType >
class RandomFeederComponent {
  typedef RandomFeederComponent <RandomFeederComponentConfigurationType > self;

  //Short name for Configuration
  typedef RandomFeederComponentConfigurationType Cfg;
  //Take a reference to the Configuration Object we get in our constructor
  Cfg & theCfg;

  mt19937 theRandomGenerator;
  uniform_real<mt19937, float> theProbabilityGenerator;
  uniform_int<mt19937, uint32_t> theLocalityGenerator;
  uniform_int<mt19937, uint32_t> theAddressGenerator;
  deque<FakeInstruction::address_type> theLastAddresses;

  LocalResizingPool<FakeInstruction> theInstructionPool;

public:
  RandomFeederComponent (Cfg & aCfg) :
    theCfg(aCfg),
    theProbabilityGenerator(theRandomGenerator, 0.0, 1.0),
    //The ?: below is neccessary because theLocalityGenerator generates a compile error if Locality = 0
    theLocalityGenerator(theRandomGenerator, 0, (theCfg.Locality.value == 0) ? 1 : theCfg.Locality.value),
    theAddressGenerator(theRandomGenerator, 0, integer_traits<FakeInstruction::address_type>::const_max)
  {}

  //Initialization
  void initialize() { }

private:
  intrusive_ptr<FakeInstruction> generate_random_instruction() {
    intrusive_ptr<FakeInstruction> random_instruction = new(theInstructionPool) FakeInstruction();
    float random_number = theProbabilityGenerator();
    if (random_number < theCfg.LoadProbability.value + theCfg.StoreProbability.value) {
      if (random_number < theCfg.StoreProbability.value) {
        random_instruction->theOpCode = FakeInstruction ::iSTORE;
      } else {
        random_instruction->theOpCode = FakeInstruction ::iLOAD;
      }
      //Again, we must avoid calling theLocalityGenerator if Locality = 0, because the
      //generator isn't smart enough to handle the 0 case.
      uint32_t random_locality = (theCfg.Locality.value == 0 ? 0 : theLocalityGenerator());
      if (random_locality == 0 || theLastAddresses.size() < random_locality - 1 ) {
        random_instruction->theAddress = theAddressGenerator();
      } else {
        random_instruction->theAddress = theLastAddresses[random_locality - 1];
      }
      if ( theLastAddresses.size() >= theCfg.Locality.value ) {
        theLastAddresses.pop_front();
      }
      theLastAddresses.push_back(random_instruction->theAddress);
    } else {
      random_instruction->theOpCode = FakeInstruction ::iNOP;
    }
    return random_instruction;
  }

public:
  //Ports

  struct InstructionOutputPort : public PullOutputPort< InstructionTransport >, AlwaysAvailable {

    template <template <class Port> class Wiring>
    static InstructionTransport pull(self & aFeeder) {
      InstructionTransport aTransport;
      aTransport.set(ArchitecturalInstructionTag, aFeeder.generate_random_instruction());
      return aTransport;
    }
  };

  typedef empty_list DriveInterfaces;

};

inline std::ostream & operator <<(std::ostream & anOstream, const FakeInstruction & anInsn) {
  switch (anInsn.theOpCode) {
    case FakeInstruction::iNOP:
      anOstream << "NOP";
      break;
    case FakeInstruction::iLOAD:
      anOstream << "Load " << anInsn.theAddress;
      break;
    case FakeInstruction::iSTORE:
      anOstream << "Store " << anInsn.theAddress;
      break;
    default:
      //Something goes here
      break;
  }
  return anOstream;
}

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION()

ParameterDefinition LoadProbabilityDef("LoadProbability", "-ldp", "0.2");
ParameterDefinition StoreProbabilityDef("StoreProbability", "-stp", "0.1");
ParameterDefinition LocalityDef("Locality", "-loc", "0");

typedef Parameter<float, LoadProbabilityDef> LoadProbability;
typedef Parameter<float, StoreProbabilityDef> StoreProbability;
typedef Parameter<uint32_t, LocalityDef> Locality;

template < class Param1 = use_default, class Param2 = use_default, class Param3 = use_default >
struct RandomFeederComponentConfiguration {
  typedef FLEXUS_MAKE_PARAMETERLIST(3, (Param1, Param2, Param3)) ArgList;

  //The LoadProbability parameter
  typename resolve < ArgList,
           RandomFeeder::LoadProbability,
           RandomFeeder::LoadProbability::Dynamic::selected >::parameter LoadProbability;

  //The StoreProbability parameter
  typename resolve < ArgList,
           RandomFeeder::StoreProbability,
           RandomFeeder::StoreProbability::Dynamic::selected >::parameter StoreProbability;

  //The LoadProbability parameter
  typename resolve < ArgList,
           RandomFeeder::Locality,
           RandomFeeder::Locality::Dynamic::selected >::parameter Locality;

}; //End RandomFeederComponentConfiguration

}  //End Namespace RandomFeeder
} //namespace Flexus

#include FLEXUS_END_COMPONENT_CONFIGURATION()
#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT RandomFeeder

