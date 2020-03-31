#include <algorithm>
#include <iostream>

#include "MapTable.hpp"
#include "RegisterType.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <core/debug/debug.hpp>
namespace nuArchARM {

std::ostream &operator<<(std::ostream &anOstream, PhysicalMap &aMap) {
  anOstream << "\n\tMappings\n\t";
  for (uint32_t i = 0; i < aMap.theMappings.size(); ++i) {
    anOstream << "r" << i << ": p" << aMap.theMappings[i] << "\t";
    if ((i & 7) == 7)
      anOstream << "\n\t";
  }
  anOstream << "\n\tFree List\n\t";
  // for(const auto& aFree: theFreeList)
  //  anOstream << 'p' << aFree << ' ';
  std::for_each(aMap.theFreeList.begin(), aMap.theFreeList.end(),
                ll::var(anOstream) << 'p' << ll::_1 << ' ');
  anOstream << std::endl;
  return anOstream;
}

} /* end namespace nuArchARM */
