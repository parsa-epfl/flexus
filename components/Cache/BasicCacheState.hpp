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

#ifndef __CACHE_BASIC_CACHE_STATE_HPP
#define __CACHE_BASIC_CACHE_STATE_HPP

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <ostream>
#include <string>
#include <vector>

#define MAX_NUM_STATES 16
#define STATE_MASK     0xF
#define PREFETCH_MASK  0x10
#define PROTECT_MASK   0x20

using namespace std;

namespace nCache {
class BasicCacheState
{
  public:
    BasicCacheState(const BasicCacheState& s)
      : val(s.val)
    {
    }

    // Statble states
    static const BasicCacheState Modified;
    static const BasicCacheState Owned;
    static const BasicCacheState Exclusive;
    static const BasicCacheState Shared;
    static const BasicCacheState Invalid;

    inline string name() const { return names()[val & STATE_MASK]; }

    inline const bool operator==(const BasicCacheState& a) const { return (((a.val ^ val) & STATE_MASK) == 0); }

    inline const bool operator!=(const BasicCacheState& a) const { return (((a.val ^ val) & STATE_MASK) != 0); }

    inline BasicCacheState& operator=(const BasicCacheState& a)
    {
        val = (val & ~STATE_MASK) | (a.val & STATE_MASK);
        return *this;
    }

    inline const bool prefetched() const { return ((val & PREFETCH_MASK) != 0); }

    inline const bool isProtected() const { return ((val & PROTECT_MASK) != 0); }

    inline const bool isValid() const { return (*this != Invalid); }

    inline void setPrefetched(bool p)
    {
        if (p) {
            val = val | PREFETCH_MASK;
        } else {
            val = val & ~PREFETCH_MASK;
        }
    }

    inline void setProtected(bool p)
    {
        if (p) {
            val = val | PROTECT_MASK;
        } else {
            val = val & ~PROTECT_MASK;
        }
    }

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version)
    {
        ar& val;
    }

    static const BasicCacheState& char2State(uint8_t c)
    {
        switch (c) {
            case 'M': return Modified; break;
            case 'O': return Owned; break;
            case 'E': return Exclusive; break;
            case 'S': return Shared; break;
            case 'I': return Invalid; break;
            default: DBG_Assert(false, (<< "Unknown state '" << c << "'")); break;
        }
        return Invalid;
    }

    static const BasicCacheState& int2State(int32_t c)
    {
        // For loading text flexpoints ONLY!
        switch (c) {
            case 7: return Modified; break;
            case 3: return Owned; break;
            case 5: return Exclusive; break;
            case 1: return Shared; break;
            case 0: return Invalid; break;
            default: DBG_Assert(false, (<< "Unknown state '" << c << "'")); break;
        }
        return Invalid;
    }

  private:
    explicit BasicCacheState()
      : val(Invalid.val)
    {
        /* Never called */ *(int*)0 = 0;
    }
    BasicCacheState(const string& name)
      : val(names().size())
    {
        names().push_back(name);
        DBG_Assert(names().size() <= MAX_NUM_STATES);
    }
    static std::vector<string>& names()
    {
        static std::vector<string> theNames;
        return theNames;
    }

    char val;

    friend std::ostream& operator<<(std::ostream& os, const BasicCacheState& state);
};
};     // namespace nCache
#endif // ! __CACHE_BASIC_CACHE_STATE_HPP