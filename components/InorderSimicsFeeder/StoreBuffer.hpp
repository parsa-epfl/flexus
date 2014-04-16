/*
  V9 Memory Op
*/

namespace Flexus {
namespace SharedTypes {
struct StoreBufferEntry {
  DoubleWord theNewValue;
  int32_t thePendingStores;

  StoreBufferEntry( DoubleWord aNewValue)
    : theNewValue(aNewValue)
    , thePendingStores(1)
  {}

  int32_t operator ++() {
    return ++thePendingStores;
  };
  int32_t operator --() {
    return --thePendingStores;
  };

};

struct StoreBuffer : public std::map<PhysicalMemoryAddress, StoreBufferEntry> {};

} //SharedTypes
} //Flexus
