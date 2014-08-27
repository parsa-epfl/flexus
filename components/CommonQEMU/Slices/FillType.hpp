#ifndef FLEXUS_SLICES__FillType_HPP_INCLUDED
#define FLEXUS_SLICES__FillType_HPP_INCLUDED

#ifdef FLEXUS_FillType_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FillType data type"
#endif
#define FLEXUS_FillType_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

enum tFillType {
  eCold,
  eReplacement,
  eCoherence,
  eDGP,
  eDMA,
  ePrefetch,
  eFetch,
  eNAW,
  eDataPrivate,
  eDataSharedRO,
  eDataSharedRW,
};

std::ostream & operator << (std::ostream & anOstream, tFillType aType);

} //End SharedTypes
} //End Flexus

#endif //FLEXUS_SLICES__FillType_HPP_INCLUDED
