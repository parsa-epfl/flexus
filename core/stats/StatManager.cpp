#include "StatManager.hpp"

#include <utility>

using namespace std;

void
StatManager::attach(string observer_name, IStatObserver* observer)
{
    observer_map_.insert(make_pair<string, IStatObserver*>(move(observer_name), move(observer)));
}

StatManager&
StatManager::get_instance()
{
    if (instance_ == nullptr) { instance_ = new StatManager; }
    return *instance_;
}