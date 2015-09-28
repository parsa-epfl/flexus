#ifndef __ABSTRACT_DIRECTORY_HPP__
#define __ABSTRACT_DIRECTORY_HPP__

#include <iostream>

namespace nCMPCache {

template<typename _State>
class AbstractLookupResult : public boost::counted_base {
public:
  virtual ~AbstractLookupResult() {}
  virtual bool found() = 0;
  virtual bool	isProtected() = 0;
  virtual void setProtected(bool val) = 0;
  virtual const _State & state() const = 0;

  virtual void addSharer(int32_t sharer) = 0;
  virtual void removeSharer(int32_t sharer) = 0;
  virtual void setSharer(int32_t sharer) = 0;
  virtual void setState(const _State & state) = 0;
};


template<typename _State, typename _EState = _State>
class AbstractDirectory {
public:
  virtual ~AbstractDirectory() {}

  typedef AbstractLookupResult<_State> LookupResult;
  typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

  virtual bool allocate(LookupResult_p lookup, MemoryAddress address, const _State & state) = 0;
  virtual LookupResult_p lookup(MemoryAddress address) = 0;
  virtual bool sameSet(MemoryAddress a, MemoryAddress b) = 0;
  virtual DirEvictBuffer<_EState> * getEvictBuffer() = 0;

  virtual bool idleWorkReady() const {
    return false;
  }

  // Return number of dir lookups required.
  virtual int  doIdleWork() const {
    return 0;
  }

  virtual bool loadState(std::istream & is) = 0;
};

}; // nCMPCache

#include <components/CMPCache/CMPCacheInfo.hpp>
#include <components/CMPCache/InfiniteDirectory.hpp>
#include <components/CMPCache/StdDirectory.hpp>
#include <components/CMPCache/TaglessDirectory.hpp>

namespace nCMPCache {

template<typename _State, typename _EState>
AbstractDirectory<_State, _EState>* constructDirectory(const CMPCacheInfo & params) {
  std::string args = params.theDirParams;

  // First, extract the Name of the type of array to consruct
  std::string::size_type loc = args.find(':', 0);
  std::string name = args.substr(0, loc);

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
    if (key != "") {
      arg_list.push_back(std::make_pair(key, value));
    }
    if (pos != std::string::npos) {
      args = args.substr(pos + 1);
    }
  } while (pos != std::string::npos);

  const std::string & type = params.theDirType;
  if (strcasecmp(type.c_str(), "std") == 0 || strcasecmp(type.c_str(), "standard") == 0) {
    return new StdDirectory<_State, _EState>(params, arg_list);
  } else if (strcasecmp(type.c_str(), "infinite") == 0 || strcasecmp(type.c_str(), "inf") == 0) {
    return new InfiniteDirectory<_State, _EState>( params, arg_list);
  } else if (strcasecmp(type.c_str(), "tagless") == 0) {
    return new TaglessDirectory<_State, _EState>( params, arg_list);
  /*} else if (strcasecmp(type.c_str(), "rt") == 0) {
    return new RTDirectory<_State, _EState>( params, arg_list);
  } else if (strcasecmp(type.c_str(), "region") == 0) {
    return new RegionDirectory<_State, _EState>( params, arg_list);
  } else if ((strcasecmp(type.c_str(), "infinite_region") == 0)
             || (strcasecmp(type.c_str(), "infiniteregion") == 0)) {
    return new InfiniteRegionDir<_State, _EState>( params, arg_list);*/
  }

  DBG_Assert(false, ( << "Failed to create instance of '" << type << "' directory. Type unknown." ));
  return nullptr;
}

}; // nCMPCache

#endif // __ABSTRACT_DIRECTORY_HPP__

