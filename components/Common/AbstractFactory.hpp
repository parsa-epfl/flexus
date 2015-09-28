#ifndef ABSTRACT_FACTORY_HPP
#define ABSTRACT_FACTORY_HPP

#include <iostream>

#include <map>
#include <list>
#include <boost/function.hpp>

template<class _AbstractType, typename _ParamType>
class AbstractFactory {
public:
  //typedef _AbstractType *(cons_func_t)(std::list< std:pair<std::string, std::string> >&);
  typedef boost::function<_AbstractType* ( std::list< std::pair<std::string, std::string> >&, const _ParamType & )> cons_func_t;

private:
  typedef std::map<std::string, cons_func_t> factory_map_t;

  static factory_map_t & factory_map() {
    static factory_map_t my_map;
    return my_map;
  };

public:
  static void registerConstructor(const std::string & name, cons_func_t fn) {
    std::pair<typename factory_map_t::iterator, bool> ret = factory_map().insert( std::make_pair(name, fn) );
    DBG_Assert(ret.second, ( << "Failed to register Constructor with name '" << name << "'" ) );
  }

  static _AbstractType * createInstance(std::string args, const _ParamType & params) {
    std::string::size_type loc = args.find(':', 0);
    std::string name = args.substr(0, loc);
    if (loc != std::string::npos) {
      args = args.substr(loc + 1);
    } else {
      args = "";
    }
    typename factory_map_t::iterator iter = factory_map().find(name);
    if (iter != factory_map().end()) {
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
          value = "";
        } else {
          key = cur_arg.substr(0, equal_pos);
          value = cur_arg.substr(equal_pos + 1);
        }
        if (key.length() > 0) {
          arg_list.push_back(std::make_pair(key, value));
        }

        if (pos != std::string::npos) {
          args = args.substr(pos + 1);
        }
      } while (pos != std::string::npos);

      return iter->second(arg_list, params);
    }
    iter = factory_map().begin();
    for (; iter != factory_map().end(); iter++) {
      std::cout << "FactoryMap contains: " << iter->first << std::endl;
    }
    DBG_Assert(false, ( << "Failed to create Instance of '" << name << "'" ) );
    return nullptr;
  }

};

template<typename _AbstractType, typename _ConcreteType, typename _ParamType>
class ConcreteFactory {
public:
  ConcreteFactory() {
    AbstractFactory<_AbstractType, _ParamType>::registerConstructor(_ConcreteType::name, &_ConcreteType::createInstance);
  };
};

#endif // ABSTRACT_FACTORY_HPP
