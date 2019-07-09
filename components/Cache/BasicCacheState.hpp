// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#ifndef __CACHE_BASIC_CACHE_STATE_HPP
#define __CACHE_BASIC_CACHE_STATE_HPP

#include <ostream>
#include <string>
#include <vector>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define MAX_NUM_STATES 16
#define STATE_MASK 0xF
#define PREFETCH_MASK 0x10
#define PROTECT_MASK 0x20

using namespace std;

namespace nCache {
class BasicCacheState {
public:
  BasicCacheState(const BasicCacheState &s) : val(s.val) {
  }

  // Statble states
  static const BasicCacheState Modified;
  static const BasicCacheState Owned;
  static const BasicCacheState Exclusive;
  static const BasicCacheState Shared;
  static const BasicCacheState Invalid;

  inline string name() const {
    return names()[val & STATE_MASK];
  }

  inline const bool operator==(const BasicCacheState &a) const {
    return (((a.val ^ val) & STATE_MASK) == 0);
  }

  inline const bool operator!=(const BasicCacheState &a) const {
    return (((a.val ^ val) & STATE_MASK) != 0);
  }

  inline BasicCacheState &operator=(const BasicCacheState &a) {
    val = (val & ~STATE_MASK) | (a.val & STATE_MASK);
    return *this;
  }

  inline const bool prefetched() const {
    return ((val & PREFETCH_MASK) != 0);
  }

  inline const bool isProtected() const {
    return ((val & PROTECT_MASK) != 0);
  }

  inline const bool isValid() const {
    return (*this != Invalid);
  }

  inline void setPrefetched(bool p) {
    if (p) {
      val = val | PREFETCH_MASK;
    } else {
      val = val & ~PREFETCH_MASK;
    }
  }

  inline void setProtected(bool p) {
    if (p) {
      val = val | PROTECT_MASK;
    } else {
      val = val & ~PROTECT_MASK;
    }
  }

  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &val;
  }

  static const BasicCacheState &char2State(uint8_t c) {
    switch (c) {
    case 'M':
      return Modified;
      break;
    case 'O':
      return Owned;
      break;
    case 'E':
      return Exclusive;
      break;
    case 'S':
      return Shared;
      break;
    case 'I':
      return Invalid;
      break;
    default:
      DBG_Assert(false, (<< "Unknown state '" << c << "'"));
      break;
    }
    return Invalid;
  }

  static const BasicCacheState &int2State(int32_t c) {
    // For loading text flexpoints ONLY!
    switch (c) {
    case 7:
      return Modified;
      break;
    case 3:
      return Owned;
      break;
    case 5:
      return Exclusive;
      break;
    case 1:
      return Shared;
      break;
    case 0:
      return Invalid;
      break;
    default:
      DBG_Assert(false, (<< "Unknown state '" << c << "'"));
      break;
    }
    return Invalid;
  }

private:
  explicit BasicCacheState() : val(Invalid.val) {
    /* Never called */ *(int *)0 = 0;
  }
  BasicCacheState(const string &name) : val(names().size()) {
    names().push_back(name);
    DBG_Assert(names().size() <= MAX_NUM_STATES);
  }
  static std::vector<string> &names() {
    static std::vector<string> theNames;
    return theNames;
  }

  char val;

  friend std::ostream &operator<<(std::ostream &os, const BasicCacheState &state);
};
};     // namespace nCache
#endif // ! __CACHE_BASIC_CACHE_STATE_HPP
