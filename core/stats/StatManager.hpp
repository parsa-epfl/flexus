
#ifndef STATMANAGER_HPP
#define STATMANAGER_HPP

#include "IStatObserver.hpp"
#include "IStatSubject.hpp"

#include <map>
#include <string>

class StatManager : public IStatSubject
{
  private:
    static StatManager* instance_;
    std::map<std::string, IStatObserver*> observer_map_;

  private:
    StatManager() = default;

  public:
    static StatManager& get_instance();
    StatManager(StatManager& instance) = delete;
    void operator=(const StatManager&) = delete;

    virtual void attach(std::string observer_name, IStatObserver* observer) override;

    void dump(const std::string filename);
};

StatManager* StatManager::instance_ = nullptr;

#endif