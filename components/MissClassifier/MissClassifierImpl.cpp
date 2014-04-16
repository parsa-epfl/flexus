#include <components/MissClassifier/MissClassifier.hpp>

#include <iostream>
#include <iomanip>

#include <boost/bind.hpp>
#include <ext/hash_map>

#include <core/flexus.hpp>
#include <core/stats.hpp>

#include <fstream>

#define DBG_DefineCategories MissClassifier
#define DBG_SetDefaultOps AddCat(MissClassifier) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT MissClassifier
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nMissClassifier {
using namespace Flexus;
using namespace Flexus::Core;

namespace Stat = Flexus::Stat;
using boost::counted_base;
using boost::intrusive_ptr;

static uint32_t theBlockSizeLog2;

class FLEXUS_COMPONENT(MissClassifier) {
  FLEXUS_COMPONENT_IMPL( MissClassifier );

  // typedefs
  typedef uint32_t block_address_t;

  // the miss classifier address map
  struct IntHash {
    std::size_t operator()(uint32_t key) const {
      key = key >> nMissClassifier::theBlockSizeLog2;
      return key;
    }
  };
  typedef __gnu_cxx::hash_map < const block_address_t, // block address
          bool,                  // probed
          IntHash
          > tClassifierMap;
  tClassifierMap theClassifierMap;

  ////////////////////// private variables
  block_address_t theBlockMask;

  ////////////////////// Miss classifier stats
  Stat::StatCounter theCold_User_Fetch;
  Stat::StatCounter theCold_OS_Fetch;
  Stat::StatCounter theCoh_User_Fetch;
  Stat::StatCounter theCoh_OS_Fetch;
  Stat::StatCounter theRepl_User_Fetch;
  Stat::StatCounter theRepl_OS_Fetch;

  Stat::StatCounter theCold_User_Read;
  Stat::StatCounter theCold_OS_Read;
  Stat::StatCounter theCoh_User_Read;
  Stat::StatCounter theCoh_OS_Read;
  Stat::StatCounter theRepl_User_Read;
  Stat::StatCounter theRepl_OS_Read;

  Stat::StatCounter theCold_User_Write;
  Stat::StatCounter theCold_OS_Write;
  Stat::StatCounter theCoh_User_Write;
  Stat::StatCounter theCoh_OS_Write;
  Stat::StatCounter theRepl_User_Write;
  Stat::StatCounter theRepl_OS_Write;

  Stat::StatCounter theCold_User_Upg;    // make sense only when restoring a checkpoint without a miss classification map present
  Stat::StatCounter theCold_OS_Upg;      // make sense only when restoring a checkpoint without a miss classification map present
  Stat::StatCounter theCoh_User_Upg;
  Stat::StatCounter theCoh_OS_Upg;
  Stat::StatCounter thePlain_User_Upg;
  Stat::StatCounter thePlain_OS_Upg;

  /////////////////////////// function implementations
private:
  uint32_t log_base2(uint32_t num) const {
    uint32_t ii = 0;
    while (num > 1) {
      ii++;
      num >>= 1;
    }
    return ii;
  }

  inline block_address_t blockAddress(const PhysicalMemoryAddress addr) const {
    return (addr & theBlockMask);
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MissClassifier)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBlockMask(0)
    , theCold_User_Fetch( statName() + "-Cold:Fetch:User")
    , theCold_OS_Fetch( statName() + "-Cold:Fetch:OS")
    , theCoh_User_Fetch( statName() + "-Coh:Fetch:User")
    , theCoh_OS_Fetch( statName() + "-Coh:Fetch:OS")
    , theRepl_User_Fetch( statName() + "-Repl:Fetch:User")
    , theRepl_OS_Fetch( statName() + "-Repl:Fetch:OS")
    , theCold_User_Read( statName() + "-Cold:Read:User")
    , theCold_OS_Read( statName() + "-Cold:Read:OS")
    , theCoh_User_Read( statName() + "-Coh:Read:User")
    , theCoh_OS_Read( statName() + "-Coh:Read:OS")
    , theRepl_User_Read( statName() + "-Repl:Read:User")
    , theRepl_OS_Read( statName() + "-Repl:Read:OS")
    , theCold_User_Write( statName() + "-Cold:Write:User")
    , theCold_OS_Write( statName() + "-Cold:Write:OS")
    , theCoh_User_Write( statName() + "-Coh:Write:User")
    , theCoh_OS_Write( statName() + "-Coh:Write:OS")
    , theRepl_User_Write( statName() + "-Repl:Write:User")
    , theRepl_OS_Write( statName() + "-Repl:Write:OS")
    , theCold_User_Upg( statName() + "-Cold:Upg:User")
    , theCold_OS_Upg( statName() + "-Cold:Upg:OS")
    , theCoh_User_Upg( statName() + "-Coh:Upg:User")
    , theCoh_OS_Upg( statName() + "-Coh:Upg:OS")
    , thePlain_User_Upg( statName() + "-Plain:Upg:User")
    , thePlain_OS_Upg( statName() + "-Plain:Upg:OS")

  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize( void ) {
    if (cfg.OnOffSwitch == false) {
      return;
    }

    theBlockMask = ~(cfg.BlockSize - 1);
    theBlockSizeLog2 = log_base2(cfg.BlockSize);

    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nMissClassifier::MissClassifierComponent::finalizeWindow, this ) );
  }

  void finalizeWindow( void ) const {
  }

  ////////////////////////
  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &, MemoryMessage & aMessage ) {
    if (cfg.OnOffSwitch == false) {
      return;
    }

    const block_address_t blockAddr = blockAddress(aMessage.address());  // the block address of the reference

    //Lookup or insert the new block in the directory
    tClassifierMap::iterator iter;
    bool is_new;
    boost::tie(iter, is_new) = theClassifierMap.insert( std::make_pair( blockAddr, false ) ); //probed is initially false
    bool & theProbed = iter->second;

    switch (aMessage.type()) {
      case MemoryMessage::FetchReq:
        if      (is_new)    {
          if (!aMessage.isPriv()) theCold_User_Fetch ++;
          else theCold_OS_Fetch ++;
        } else if (theProbed) {
          if (!aMessage.isPriv()) theCoh_User_Fetch ++;
          else theCoh_OS_Fetch ++;
        } else                {
          if (!aMessage.isPriv()) theRepl_User_Fetch ++;
          else theRepl_OS_Fetch ++;
        }
        theProbed = false;
        break;

      case MemoryMessage::ReadReq:
        if      (is_new)    {
          if (!aMessage.isPriv()) theCold_User_Read ++;
          else theCold_OS_Read ++;
        } else if (theProbed) {
          if (!aMessage.isPriv()) theCoh_User_Read ++;
          else theCoh_OS_Read ++;
        } else                {
          if (!aMessage.isPriv()) theRepl_User_Read ++;
          else theRepl_OS_Read ++;
        }
        theProbed = false;
        break;

      case MemoryMessage::WriteReq:
        if      (is_new)    {
          if (!aMessage.isPriv()) theCold_User_Write ++;
          else theCold_OS_Write ++;
        } else if (theProbed) {
          if (!aMessage.isPriv()) theCoh_User_Write ++;
          else theCoh_OS_Write ++;
        } else                {
          if (!aMessage.isPriv()) theRepl_User_Write ++;
          else theRepl_OS_Write ++;
        }
        theProbed = false;
        break;

      case MemoryMessage::UpgradeReq:
        if      (is_new)    {
          if (!aMessage.isPriv()) theCold_User_Upg ++;
          else theCold_OS_Upg ++;
        } else if (theProbed) {
          if (!aMessage.isPriv()) theCoh_User_Upg ++;
          else theCoh_OS_Upg ++;
        } else                {
          if (!aMessage.isPriv()) thePlain_User_Upg ++;
          else thePlain_OS_Upg ++;
        }
        theProbed = false;
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
        theProbed = false;
        break;

      case MemoryMessage::ReturnReq:
        break;

      case MemoryMessage::Invalidate:
      case MemoryMessage::Downgrade:
        theProbed = true;
        break;

      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
    }
  }

  void drive( interface::UpdateStatsDrive const & )
  { }

  ///////////////////////////////////////////////////////////////
  // Checkpointing

  void saveState(std::string const & aDirName) {
    if (cfg.OnOffSwitch == false) {
      return;
    }

    std::string fname( aDirName );
    fname += "/" + statName();
    std::ofstream ofs( fname.c_str() );

    // Write the block mask and the number of entries
    ofs << theBlockMask << " " << theBlockSizeLog2 << " " << theClassifierMap.size() << std::endl;

    // Write out each directory entry
    for ( tClassifierMap::iterator iter = theClassifierMap.begin();
          iter != theClassifierMap.end();
          iter++ ) {
      const uint32_t theAddress = iter->first;
      const bool          theProbed  = iter->second;

      ofs << "{ "
          << theAddress << " "
          << theProbed  << " }"
          << std::endl;
    }
    ofs.close();
  }

  void loadState( std::string const & aDirName ) {
    if (cfg.OnOffSwitch == false) {
      return;
    }

    std::string fname( aDirName );
    fname += "/" + statName();
    std::ifstream ifs( fname.c_str() );
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty classifier map. " )  );
      return;
    }

    theClassifierMap.clear(); // empty the miss classifier map

    // Read the block mask and the number of entries
    int32_t mapSize = 0;
    ifs >> theBlockMask >> theBlockSizeLog2 >> mapSize;
    DBG_Assert ( mapSize >= 0 );

    // Read in each block
    for ( int32_t i = 0; i < mapSize; i++ ) {
      char paren;

      ifs >> paren;
      DBG_Assert ( (paren == '{'), ( << "Expected '{' when loading the ClassifierMap from checkpoint" ) );

      uint32_t theAddress;
      bool          theProbed;

      ifs >> theAddress
          >> theProbed;

      ifs >> paren;
      DBG_Assert ( (paren == '}'), ( << "Expected '}' when loading CMPDir from checkpoint" ) );

      tClassifierMap::iterator iter;
      bool is_new;
      boost::tie(iter, is_new) = theClassifierMap.insert( std::make_pair( theAddress, theProbed ) );
      DBG_Assert(is_new);
    }

    ifs.close();
  }

};  // end class MissClassifier

} // end namespace nMissClassifier

FLEXUS_COMPONENT_INSTANTIATOR( MissClassifier, nMissClassifier);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MissClassifier

#define DBG_Reset
#include DBG_Control()
