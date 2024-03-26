//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED
#define FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED

#include <core/types.hpp>

#include <core/aux_/wiring/channels.hpp>

namespace Flexus {
namespace Core {

struct ComponentInterface {

  // This allows components to have push output ports
  template <class Port, typename AvailableFn, typename ManipulateFn>
  static Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>
  get_channel(Port const &, AvailableFn avail, ManipulateFn manip,
              Flexus::Core::index_t aComponentIndex) {
    return Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>(avail, manip, aComponentIndex);
  }

  template <class Port, typename AvailableFn, typename ManipulateFn>
  static Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>
  get_channel_array(Port const &, AvailableFn avail, ManipulateFn manip,
                    Flexus::Core::index_t aComponentIndex, Flexus::Core::index_t aPortIndex,
                    Flexus::Core::index_t aPortWidth) {

    DBG_Assert(aPortIndex < aPortWidth, (<< "PortIndex: " << aPortIndex << " Width: " << aPortWidth));

    return Flexus::Core::aux_::port<Port, AvailableFn, ManipulateFn>(
        avail, manip, aComponentIndex * aPortWidth + aPortIndex);
  }

  // All components must provide the following members
  virtual void initialize() = 0;
  // added by PLotfi
  virtual void finalize() = 0;
  // end PLotfi
  virtual bool isQuiesced() const = 0;
  virtual void saveState(std::string const &aDirectory) = 0;
  virtual void loadState(std::string const &aDirectory) = 0;
  virtual std::string name() const = 0;
  virtual ~ComponentInterface() {
  }
};

} // namespace Core
} // namespace Flexus

#endif // FLEXUS__COMPONENT_INTERFACE_HPP_INCLUDED
