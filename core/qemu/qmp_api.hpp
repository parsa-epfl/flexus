#ifndef FLEXUS_QEMU_QMP_API_HPP_INCLUDED
#define FLEXUS_QEMU_QMP_API_HPP_INCLUDED

#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace Flexus {
namespace Qemu {
namespace API {
#include <core/qemu/api.h>
} // namespace API
} // namespace Qemu
} // namespace Flexus

using namespace Flexus::Core;
using namespace Flexus::Qemu::API;

class qmp_flexus_i;
typedef std::map<qmp_flexus_cmd_t, qmp_flexus_i&> qmpFactories;
qmpFactories qmpCommands;

template<typename Out>
static void
split(const std::string& s, char delim, Out result)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

static std::vector<std::string>
split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

// ---------------qmp call products
class qmp_flexus_i
{
    typedef std::vector<std::string> arguments;

  public:
    virtual void execute(std::string anArgs) = 0;

  protected:
    arguments theArgsVector;
};

class qmp_write_measurement : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 2)
            theFlexus->writeMeasurement(theArgsVector[0], theArgsVector[1]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_write_measurement_;

class qmp_terminate_simulation : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->terminateSimulation(); }

} qmp_terminate_simulation_;

class qmp_default : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { DBG_(Crit, (<< "Wrong qmp command!")); }

} qmp_default_;

// ---------------qmp box
class qmp_not_implemented : public exception
{
    const char* what() const throw() { return "qmp flexus Exception"; }
};

qmp_flexus_i&
qmp(qmp_flexus_cmd_t aCMD)
{

    switch (aCMD) {
        case QMP_FLEXUS_WRITEMEASUREMENT: return qmp_write_measurement_;
        case QMP_FLEXUS_TERMINATESIMULATION: return qmp_terminate_simulation_;
        default: throw qmp_not_implemented();
    }
}

#endif // FLEXUS_QEMU_QMP_API_HPP_INCLUDED
