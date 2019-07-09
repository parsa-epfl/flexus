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

#ifndef __ABSTRACT_REGION_DIRECTORY_HPP__
#define __ABSTRACT_REGION_DIRECTORY_HPP__

#include <components/CMPCache/AbstractDirectory.hpp>

#include <iostream>

namespace nCMPCache {

template <typename _State> class AbstractRegionLookupResult : public AbstractLookupResult<_State> {
public:
  virtual ~AbstractRegionLookupResult() {
  }
  virtual int32_t owner() const = 0;

  virtual const _State &regionState(int32_t i) const = 0;
  virtual const std::vector<_State> &regionState() const = 0;

  virtual MemoryAddress regionTag() const = 0;

  virtual void setRegionOwner(int32_t new_owner) = 0;
  virtual void setRegionState(const std::vector<_State> &new_state) = 0;
  virtual void setRegionSharerState(int32_t sharer, boost::dynamic_bitset<uint64_t> &presence,
                                    boost::dynamic_bitset<uint64_t> &exclusive) = 0;

  virtual bool emptyRegion() = 0;
};

template <typename _State> class AbstractRegionEvictBuffer : public DirEvictBuffer<_State> {
public:
  AbstractRegionEvictBuffer(int32_t size) : DirEvictBuffer<_State>(size) {
  }
  virtual ~AbstractRegionEvictBuffer() {
  }
};

template <typename _State, typename _EState = _State>
class AbstractRegionDirectory : public AbstractDirectory<_State, _EState> {
public:
  virtual ~AbstractRegionDirectory() {
  }

  typedef AbstractRegionLookupResult<_State> RegLookupResult;
  typedef typename boost::intrusive_ptr<RegLookupResult> RegLookupResult_p;

  virtual RegLookupResult_p nativeLookup(MemoryAddress address) = 0;
  virtual AbstractRegionEvictBuffer<_EState> *getRegionEvictBuffer() = 0;

  virtual int32_t blocksPerRegion() = 0;
  virtual MemoryAddress getRegion(MemoryAddress addr) = 0;
};

}; // namespace nCMPCache

#endif // __ABSTRACT_REGION_DIRECTORY_HPP__
