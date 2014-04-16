/*! \file RdramBank.hpp
 * \brief
 *
 *  Brief description goes here.
 *
 * Revision History:
 *     cfchen      23 Apr 03 - Initial Revision
 */

#include <deque>
#include <vector>
#include <iterator>
#include "RdramMagic.hpp"

namespace Flexus {
namespace Rdram {

using namespace Flexus::Typelist;
using Flexus::Debug::Debug;

class RdramBank {

public:
  RdramBank() {
  }

  ~RdramBank() {
  }

  int32_t read(uint32_t address) {
    return 1;
  }

  int32_t write(uint32_t address) {
    return 1;
  }

private:

  // bank state
  int32_t id;
  int32_t lastAccessType;
  RdramCycles lastAccessed;

  //statistics
  int64_t accesses,
          reads,
          writes;

};  // end class RdramBank

}  // end namespace Rdram
}  // end namespace Flexus

