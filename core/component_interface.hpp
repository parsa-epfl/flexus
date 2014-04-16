#ifndef FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED
#define FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED

#include <core/types.hpp>
#include <core/aux_/wiring/channels.hpp>

namespace Flexus {
namespace Core {

struct ComponentInterface {

  //This allows components to have push output ports
  template <class Port, typename AvailableFn, typename ManipulateFn >
  static Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>
  get_channel(Port const &, AvailableFn avail, ManipulateFn manip, Flexus::Core::index_t aComponentIndex) {
    return Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>(avail, manip, aComponentIndex);
  }

  template <class Port, typename AvailableFn, typename ManipulateFn >
  static Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>
  get_channel_array(Port const &, AvailableFn avail, ManipulateFn manip, Flexus::Core::index_t aComponentIndex, Flexus::Core::index_t aPortIndex, Flexus::Core::index_t aPortWidth) {
    DBG_Assert( aPortIndex < aPortWidth, ( << "PortIndex: " << aPortIndex << " Width: " << aPortWidth ) );
    return Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn > (avail, manip, aComponentIndex * aPortWidth + aPortIndex );
  }

  //All components must provide the following members
  virtual void initialize() = 0;
  // added by PLotfi
  virtual void finalize() = 0;
  // end PLotfi
  virtual bool isQuiesced() const = 0;
  virtual void saveState(std::string const & aDirectory) = 0;
  virtual void loadState(std::string const & aDirectory) = 0;
  virtual std::string name() const = 0;
  virtual ~ComponentInterface() {}

};

}//namespace ComponentInterface
}//namespace Flexus

#endif //FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED

