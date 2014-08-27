#ifndef FLEXUS_SLICES__FillLevel_HPP_INCLUDED
#define FLEXUS_SLICES__FillLevel_HPP_INCLUDED

#ifdef FLEXUS_FillLevel_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FillLevel data type"
#endif
#define FLEXUS_FillLevel_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

enum tFillLevel {
  eUnknown,
  eL1,
  eL2,
  eL3,
  eLocalMem,
  eRemoteMem,
  ePrefetchBuffer,
  ePeerL1Cache,
  eL2Prefetcher,
  eL1I,
  eCore,
  ePeerL2,
  eDirectory,
  NumFillLevels
};

std::ostream & operator << (std::ostream & anOstream, tFillLevel aType);
std::string fillLevelName(tFillLevel aType);

} //End SharedTypes
} //End Flexus

#endif //FLEXUS_SLICES__FillLevel_HPP_INCLUDED
