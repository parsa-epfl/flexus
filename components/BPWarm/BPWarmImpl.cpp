#include <components/BPWarm/BPWarm.hpp>

#include <components/CommonQEMU/Slices/ArchitecturalInstruction.hpp>

#define FLEXUS_BEGIN_COMPONENT BPWarm
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/CommonQEMU/BranchPredictor.hpp>

namespace nBPWarm {

using namespace Flexus;
using namespace Flexus::SharedTypes;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(BPWarm) {
  FLEXUS_COMPONENT_IMPL(BPWarm);

  std::unique_ptr<FastBranchPredictor> theBranchPredictor;

  std::vector< std::vector< VirtualMemoryAddress > > theFetchAddress;
  std::vector< std::vector< BPredState > >           theFetchState;
  std::vector< std::vector< eBranchType > >          theFetchType;
  std::vector< std::vector< bool > >                 theFetchAnnul;
  std::vector< bool >                                theOne;

  std::pair<eBranchType, bool> decode( uint32_t opcode ) {
#if FLEXUS_TARGET_IS(v9)
    uint32_t op = opcode >> 30 & 3;
    switch (op) {
      case 0: {
        uint32_t op2 = (opcode >> 22) & 0x7;
        switch (op2) {
          case 1:
          case 2:
          case 3:
          case 5:
          case 6:
            if ( opcode & 0x20000000) {
              return std::make_pair(kConditional, true);
            } else {
              return std::make_pair(kConditional, false);
            }
          default:
            return std::make_pair(kNonBranch, false);
        }
      }
      case 1:
        return std::make_pair(kUnconditional, false);//kCall
      case 2: {
        uint32_t op3 = (opcode >> 19) & 0x3F;
        if (op3 == 0x38 /*jmpl*/ || op3 == 0x39 /*return*/) {
          return std::make_pair(kUnconditional, false);
        } else {
          return std::make_pair(kNonBranch, false);
        }
      }
      case 3: //No branches
      default:
        return std::make_pair(kNonBranch, false);
    }
#else
    return std::make_pair(kNonBranch, false);
#endif
  }

  void doUpdate(VirtualMemoryAddress theActual,
                index_t anIndex) {
    const bool anOne = theOne[anIndex];
    VirtualMemoryAddress pc = theFetchAddress[anIndex][!anOne];
    eDirection dir = kNotTaken;
    VirtualMemoryAddress target = VirtualMemoryAddress(0);
    if ( theFetchType[anIndex][!anOne] == kConditional) {
      if (theFetchAnnul[anIndex][!anOne]) {
        //For annulled branches, Fetch1 should be theFetch2 + 8
        if (theFetchAddress[anIndex][anOne] == theFetchAddress[anIndex][!anOne] + 8) {
          dir = kNotTaken ;
        } else {
          dir  = kTaken ;
          target = theActual;
        }
      } else {
        //For non-annulled branches, theActual should be theFetch2 + 8
        if (theActual == theFetchAddress[anIndex][!anOne] + 8) {
          dir = kNotTaken ;
        } else {
          dir = kTaken ;
          target = theActual;
        }
      }
    } else if ( theFetchType[anIndex][!anOne] == kNonBranch ) {
      dir = kNotTaken ;
      target = VirtualMemoryAddress(0);
    } else {
      dir = kTaken ;
      target = theActual;
    }

    theBranchPredictor->feedback( pc, theFetchType[anIndex][!anOne], dir, target, theFetchState[anIndex][!anOne]);
  }

  void doPredict(index_t anIndex) {
    const bool anOne = theOne[anIndex];
    theBranchPredictor->predict( theFetchAddress[anIndex][anOne], theFetchState[anIndex][anOne] );
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(BPWarm)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
  }

  void initialize() {
    theFetchAddress.resize(cfg.Cores);
    theFetchState.resize(cfg.Cores);
    theFetchType.resize(cfg.Cores);
    theFetchAnnul.resize(cfg.Cores);
    theOne.resize(cfg.Cores);

    for (int32_t i = 0; i < cfg.Cores; i++) {
      theFetchAddress[i].resize(2);
      theFetchState[i].resize(2);
      theFetchType[i].resize(2);
      theFetchAnnul[i].resize(2);

      theFetchAddress[i][0] =  VirtualMemoryAddress(0);
      theFetchAddress[i][1] =  VirtualMemoryAddress(0);
      theFetchType[i][0] =  kNonBranch;
      theFetchType[i][1] =  kNonBranch;
      theOne[i] = false;
    }

    theBranchPredictor.reset( FastBranchPredictor::combining(statName(), flexusIndex()) );
  }

  void finalize() {}

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
    theBranchPredictor->saveState(aDirName);
  }

  void loadState(std::string const & aDirName) {
    theBranchPredictor->loadState(aDirName);
  }

public:
  ///////////////// InsnIn port
  bool available( interface::InsnIn const &,
                  index_t anIndex) {
    return FLEXUS_CHANNEL_ARRAY( InsnOut, anIndex ).available();
  }
  void push( interface::InsnIn const &,
             index_t           anIndex,
             InstructionTransport & anInstruction) {
    bool anOne = theOne[anIndex];
    if (theFetchType[anIndex][!anOne] != kNonBranch) {
      doUpdate(anInstruction[ArchitecturalInstructionTag]->virtualInstructionAddress(), anIndex);
    }
    theOne[anIndex] = !theOne[anIndex];
    anOne = theOne[anIndex];

    eBranchType aFetchType;
    bool aFetchAnnul;
    std::tie(aFetchType, aFetchAnnul) = decode( anInstruction[ArchitecturalInstructionTag]->opcode() );
    theFetchType[anIndex][anOne] = aFetchType;
    theFetchAnnul[anIndex][anOne] = aFetchAnnul;
    // std::tie(theFetchType[anIndex][anOne], theFetchAnnul[anIndex][anOne]) = decode( anInstruction[ArchitecturalInstructionTag]->opcode() );

    if (theFetchType[anIndex][anOne] != kNonBranch) {
      //save the other relevant state and make a prediction
      theFetchAddress[anIndex][anOne] = anInstruction[ArchitecturalInstructionTag]->virtualInstructionAddress();
      theFetchState[anIndex][anOne].thePredictedType =  kNonBranch;
      doPredict(anIndex);
    }

    FLEXUS_CHANNEL_ARRAY( InsnOut, anIndex ) << anInstruction;
  }

  ///////////////// ITraceIn port
  bool available( interface::ITraceIn const &,
                  index_t anIndex) {
    return true;
  }
  void push( interface::ITraceIn const &,
             index_t           anIndex,
             std::pair< uint64_t, uint32_t > & aPCOpcodePair) {
    bool anOne = theOne[anIndex];
    if (theFetchType[anIndex][!anOne] != kNonBranch) {
      doUpdate(VirtualMemoryAddress(aPCOpcodePair.first), anIndex);
    }
    theOne[anIndex] = !theOne[anIndex];
    anOne = theOne[anIndex];

    eBranchType aFetchType;
    bool aFetchAnnul;
    std::tie(aFetchType, aFetchAnnul) = decode( aPCOpcodePair.second );
    theFetchType[anIndex][anOne] = aFetchType;
    theFetchAnnul[anIndex][anOne] = aFetchAnnul;
    // std::tie(theFetchType[anIndex][anOne], theFetchAnnul[anIndex][anOne]) = decode( aPCOpcodePair.second );

    if (theFetchType[anIndex][anOne] != kNonBranch) {
      //save the other relevant state and make a prediction
      theFetchAddress[anIndex][anOne] = VirtualMemoryAddress(aPCOpcodePair.first);
      theFetchState[anIndex][anOne].thePredictedType =  kNonBranch;
      doPredict(anIndex);
    }
  }

  ///////////////// ITraceInModern port
  bool available( interface::ITraceInModern const &,
                  index_t anIndex) {
    return true;
  }
  void push( interface::ITraceInModern const &,
             index_t           anIndex,
             std::pair< uint64_t, std::pair< uint32_t, uint32_t> > & aPCAndTypeAndAnnulPair) {
    bool anOne = theOne[anIndex];

    uint64_t aVirtualPC = aPCAndTypeAndAnnulPair.first;
    std::pair< eBranchType, bool > aTypeAndAnnulPair = std::pair< eBranchType, bool >(
							    (eBranchType)aPCAndTypeAndAnnulPair.second.first,
							    (bool)aPCAndTypeAndAnnulPair.second.second );

    if (theFetchType[anIndex][!anOne] != kNonBranch) {
      doUpdate(VirtualMemoryAddress(aVirtualPC), anIndex);
    }
    theOne[anIndex] = !theOne[anIndex];
    anOne = theOne[anIndex];

    eBranchType aFetchType;
    bool aFetchAnnul;
    std::tie(aFetchType, aFetchAnnul) = aTypeAndAnnulPair;
    theFetchType[anIndex][anOne] = aFetchType;
    theFetchAnnul[anIndex][anOne] = aFetchAnnul;

    if (theFetchType[anIndex][anOne] != kNonBranch) {
      //save the other relevant state and make a prediction
      theFetchAddress[anIndex][anOne] = VirtualMemoryAddress(aVirtualPC);
      theFetchState[anIndex][anOne].thePredictedType =  kNonBranch;
      doPredict(anIndex);
    }
  }
};

}//End namespace nBPWarm

FLEXUS_COMPONENT_INSTANTIATOR( BPWarm, nBPWarm );

FLEXUS_PORT_ARRAY_WIDTH( BPWarm, InsnIn )     {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( BPWarm, InsnOut )    {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( BPWarm, ITraceIn )   {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( BPWarm, ITraceInModern )   {
  return (cfg.Cores);
}
#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT BPWarm
