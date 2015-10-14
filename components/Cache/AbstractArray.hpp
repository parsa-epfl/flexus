#ifndef _ABSTRACT_ARRAY_HPP
#define _ABSTRACT_ARRAY_HPP

#include <exception>
#include <iostream>
#include <stdlib.h>
#include <limits.h>

#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

namespace nCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef uint32_t BlockOffset;

class InvalidCacheAccessException : public std::exception {};

// The output of a cache lookup
// derived from counted_base so we work with intrusive_ptr
template<typename _State>
class AbstractLookupResult : public boost::counted_base {
public:
  virtual ~AbstractLookupResult () {}
  AbstractLookupResult() {}

  virtual const _State & state( void ) const = 0;
  virtual void setState( const _State & aNewState) = 0;
  virtual void setProtected( bool val) = 0;
  virtual void setPrefetched( bool val) = 0;

  virtual bool hit  ( void ) const = 0;
  virtual bool miss ( void ) const = 0;
  virtual bool found( void ) const = 0;
  virtual bool valid( void ) const = 0;

  virtual MemoryAddress blockAddress ( void ) const = 0;

}; // class AbstractLookupResult

template<typename _State>
class AbstractArray {
public:

  virtual ~AbstractArray () {}
  AbstractArray() {}

  typedef boost::intrusive_ptr<AbstractLookupResult<_State> > LookupResult_p;

  // Main array lookup function
  virtual LookupResult_p operator[] ( const MemoryAddress & anAddress ) = 0;

  virtual LookupResult_p allocate ( LookupResult_p lookup, const MemoryAddress & anAddress ) = 0;
  virtual bool canAllocate(LookupResult_p lookup, const MemoryAddress & anAddress ) = 0;

  // Fancy names for makeMRU and makeLRU
  // invalidateBlock only changes replacement order, it DOES NOT change the block's state to invalid
  virtual void recordAccess ( LookupResult_p lookup) = 0;
  virtual void invalidateBlock ( LookupResult_p lookup) = 0;

  virtual std::function<bool (MemoryAddress a, MemoryAddress b)> setCompareFn() const = 0;

  // Checkpoint reading/writing functions
  virtual bool saveState ( std::ostream & s ) = 0;
  virtual bool loadState ( std::istream & s, int32_t anIndex, bool aTextFlexpoint ) = 0;

  // Addressing helper functions
  MemoryAddress blockAddress ( MemoryAddress const & anAddress ) const {
    return MemoryAddress ( anAddress & ~(blockOffsetMask) );
  }

  BlockOffset blockOffset ( MemoryAddress const & anAddress ) const {
    return BlockOffset ( anAddress & blockOffsetMask );
  }

  virtual uint32_t freeEvictionResources() {
    return UINT_MAX;
  }
  virtual bool evictionResourcePressure() {
    return false;
  }
  virtual bool evictionResourcesAvailable() {
    return true;
  }
  virtual bool evictionResourcesEmpty() {
    return true;
  }
  virtual void reserveEvictionResource() { }
  virtual void unreserveEvictionResource() { }
  virtual std::pair<_State, MemoryAddress> getPreemptiveEviction() = 0;

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) {
    return false;
  }
  virtual std::list<MemoryAddress>  getSetTags(MemoryAddress addr) {
    DBG_Assert(false, ( << "Derived class does not implement getSetTags() function." ));
    return std::list<MemoryAddress>();
  }

  virtual uint64_t getSet(MemoryAddress const & addr) const = 0;
  virtual int32_t requestsPerSet() const = 0;

protected:

  MemoryAddress blockOffsetMask;

}; // class AbstractArray

};  // namespace nCache

#include <components/Cache/StdArray.hpp>
#include <components/Cache/RTArray.hpp>

namespace nCache {

/*
 * Construct an array based on the configuration given
 *
 * The configuraion has the following format:
 *
 *      <Type>[:[<key>=<value>[:<key>=<value>]]
 *
 * <Type> is the name of the array type to be created
 * and it is followed by an optional series of key,value pairs
 * that describe a configuration appropriate for that type
 *
 */
template<typename _State, const _State & _Default>
AbstractArray<_State>* constructArray(std::string & anArrayConfiguration, const std::string & theName, int32_t theNodeId, int32_t theBlockSize) {
  std::string & args = anArrayConfiguration;

  // First, extract the Name of the type of array to consruct
  std::string::size_type loc = args.find(':', 0);
  std::string name = args.substr(0, loc);
  if (loc != std::string::npos) {
    args = args.substr(loc + 1);
  } else {
    args = "";
  }

  // Now create a list of key,value pairs
  std::list< std::pair<std::string, std::string> > arg_list;
  std::string key;
  std::string value;
  std::string::size_type pos = 0;
  do {
    pos = args.find(':', 0);
    std::string cur_arg = args.substr(0, pos);
    std::string::size_type equal_pos = cur_arg.find('=', 0);
    if (equal_pos == std::string::npos) {
      key = cur_arg;
      value = "1";
    } else {
      key = cur_arg.substr(0, equal_pos);
      value = cur_arg.substr(equal_pos + 1);
    }
    arg_list.push_back(std::make_pair(key, value));

    if (pos != std::string::npos) {
      args = args.substr(pos + 1);
    }
  } while (pos != std::string::npos);

  // Now construct an array of the appropriate type
  // BlockSize is always passed separately to avoid specifying it more than once
  if (name == "std" || name == "Std" || name == "STD") {
    return new StdArray<_State, _Default>(theBlockSize, arg_list);
  } else if (name == "RegionTracker" || name == "RT" || name == "rt") {
    return new RTArray<_State, _Default>(theName, theNodeId, theBlockSize, arg_list);
  }

  DBG_Assert(false, ( << "Failed to create Instance of '" << name << "'" ) );
  return nullptr;
}

};  // namespace nCache

#endif /* _ABSTRACT_ARRAY_HPP */
