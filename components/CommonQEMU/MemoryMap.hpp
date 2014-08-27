#ifndef FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED
#define FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED

#include <core/types.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>

namespace Flexus {
namespace SharedTypes {

typedef Flexus::Core::index_t node_id_t;

struct MemoryMap : public boost::counted_base {
  static boost::intrusive_ptr<MemoryMap> getMemoryMap();
  static boost::intrusive_ptr<MemoryMap> getMemoryMap(Flexus::Core::index_t aRequestingNode);

  enum AccessType {
    Read
    , Write
    , NumAccessTypes /* Must be last */
  };

  static std::string const & AccessType_toString(AccessType anAccessType) {
    static std::string access_type_names[] = {
      "Read "
      , "Write"
    };
    //xyzzy
    DBG_Assert((anAccessType < NumAccessTypes));
    return access_type_names[anAccessType];
  }

  virtual bool isCacheable(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
  virtual bool isMemory(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
  virtual bool isIO(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
  virtual node_id_t node(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
  virtual void loadState(std::string const &) = 0;
  virtual void recordAccess(Flexus::SharedTypes::PhysicalMemoryAddress const &, AccessType anAccessType) = 0;

};

} //SharedTypes
} //Flexus

#endif //FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED
