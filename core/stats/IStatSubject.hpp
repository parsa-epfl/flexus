#ifndef ISTATSUBJECT_HPP
#define ISTATSUBJECT_HPP

#include "IStatObserver.hpp"

#include <string>

class IStatSubject
{

  public:
    virtual ~IStatSubject(){};
    virtual void attach(std::string observer_name, IStatObserver* observer) = 0;
};

#endif