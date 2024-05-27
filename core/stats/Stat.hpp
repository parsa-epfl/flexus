#ifndef STAT_HPP
#define STAT_HPP

#include "IStatObserver.hpp"
#include "StatManager.hpp"

#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <utility>

class Stat : public IStatObserver
{
  private:
    std::map<std::string, uint64_t> stats_map;

  public:
    template<typename Iterable>
    Stat(const std::string name, const Iterable& list_entries)
    {
        for (const auto c : list_entries) {
            stats_map.insert(std::make_pair<std::string, uint64_t>(c, 0));
        }

        StatManager::get_instance().attach(name, this);
    };

    void increment(const std::string key) { stats_map.at(key) += 1; }
    void add(const std::string key, uint64_t value) { stats_map.at(key) += value; }
};

#endif