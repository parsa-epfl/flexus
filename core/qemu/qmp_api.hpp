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

class qmp_print_cycle_count : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->printCycleCount(); }

} qmp_print_cycle_count_;

class qmp_set_stop_cycle : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setStopCycle(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_stop_cycle_;

class qmp_set_stat_interval : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setStatInterval(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_stat_interval_;

class qmp_set_region_interval : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setRegionInterval(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_region_interval_;

class qmp_set_break_cpu : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setBreakCPU(stoi(theArgsVector[0]));
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_break_cpu_;

class qmp_set_break_insn : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setBreakInsn(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_break_insn_;

class qmp_set_profile_interval : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setProfileInterval(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_profile_interval_;

class qmp_set_timestap_interval : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');

        if (theArgsVector.size() == 1)
            theFlexus->setTimestampInterval(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_timestap_interval_;

class qmp_print_profile : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->printProfile(); }

} qmp_print_profile_;

class qmp_reset_profile : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->resetProfile(); }

} qmp_reset_profile_;

class qmp_write_profile : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->writeProfile(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_write_profile_;

class qmp_print_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->printDebugConfiguration(); }

} qmp_print_configuration_;

class qmp_write_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->writeConfiguration(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_write_configuration_;

class qmp_parse_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->parseConfiguration(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_parse_configuration_;

class qmp_set_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 2)
            theFlexus->setConfiguration(theArgsVector[0], theArgsVector[1]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_configuration_;

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

class qmp_enter_fastmode : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->enterFastMode(); }

} qmp_enter_fastmode_;

class qmp_leave_fastmode : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->leaveFastMode(); }

} qmp_leave_fastmode_;

class qmp_quiesce : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->quiesce(); }

} qmp_quiesce_;

class qmp_do_load : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->doLoad(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_do_load_;

class qmp_do_save : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->doSave(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_do_save_;

class qmp_reload_debug_cfg : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->reloadDebugCfg(); }

} qmp_reload_debug_cfg_;

class qmp_add_debug_cfg : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->addDebugCfg(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_add_debug_cfg_;

class qmp_set_debug : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->setDebug(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_set_debug_;

class qmp_enable_category : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->enableCategory(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_enable_category_;

class qmp_disable_category : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->disableCategory(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_disable_category_;

class qmp_list_categories : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->listCategories(); }

} qmp_list_categories_;

class qmp_enable_component : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 2)
            theFlexus->enableComponent(theArgsVector[0], theArgsVector[1]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_enable_component_;

class qmp_disable_component : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 2)
            theFlexus->disableComponent(theArgsVector[0], theArgsVector[1]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_disable_component_;

class qmp_list_components : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->listComponents(); }

} qmp_list_components_;

class qmp_print_debug_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->printDebugConfiguration(); }

} qmp_print_debug_configuration_;

class qmp_write_debug_configuration : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->writeDebugConfiguration(theArgsVector[0]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_write_debug_configuration_;

class qmp_terminate_simulation : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override { theFlexus->terminateSimulation(); }

} qmp_terminate_simulation_;

class qmp_log : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 3)
            theFlexus->log(theArgsVector[0], theArgsVector[1], theArgsVector[2]);
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_log_;

class qmp_print_mmu : public qmp_flexus_i
{

    virtual void execute(std::string anArgs) override
    {
        if (!anArgs.empty()) theArgsVector = split(anArgs, ':');
        if (theArgsVector.size() == 1)
            theFlexus->printMMU(stoi(theArgsVector[0]));
        else
            DBG_(Crit, (<< "Wrong number of arguments."));
    }

} qmp_print_mmu_;

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
        case QMP_FLEXUS_PRINTCYCLECOUNT: return qmp_print_cycle_count_;
        case QMP_FLEXUS_SETSTOPCYCLE: return qmp_set_stop_cycle_;
        case QMP_FLEXUS_SETSTATINTERVAL: return qmp_set_stat_interval_;
        case QMP_FLEXUS_SETREGIONINTERVAL: return qmp_set_region_interval_;
        case QMP_FLEXUS_SETBREAKCPU: return qmp_set_break_cpu_;
        case QMP_FLEXUS_SETBREAKINSN: return qmp_set_break_insn_;
        case QMP_FLEXUS_SETPROFILEINTERVAL: return qmp_set_profile_interval_;
        case QMP_FLEXUS_SETTIMESTAMPINTERVAL: return qmp_set_timestap_interval_;
        case QMP_FLEXUS_PRINTPROFILE: return qmp_print_profile_;
        case QMP_FLEXUS_RESETPROFILE: return qmp_reset_profile_;
        case QMP_FLEXUS_WRITEPROFILE: return qmp_write_profile_;
        case QMP_FLEXUS_PRINTCONFIGURATION: return qmp_print_configuration_;
        case QMP_FLEXUS_WRITECONFIGURATION: return qmp_write_configuration_;
        case QMP_FLEXUS_PARSECONFIGURATION: return qmp_parse_configuration_;
        case QMP_FLEXUS_SETCONFIGURATION: return qmp_set_configuration_;
        case QMP_FLEXUS_WRITEMEASUREMENT: return qmp_write_measurement_;
        case QMP_FLEXUS_ENTERFASTMODE: return qmp_enter_fastmode_;
        case QMP_FLEXUS_LEAVEFASTMODE: return qmp_leave_fastmode_;
        case QMP_FLEXUS_QUIESCE: return qmp_quiesce_;
        case QMP_FLEXUS_DOLOAD: return qmp_do_load_;
        case QMP_FLEXUS_DOSAVE: return qmp_do_save_;
        case QMP_FLEXUS_RELOADDEBUGCFG: return qmp_reload_debug_cfg_;
        case QMP_FLEXUS_ADDDEBUGCFG: return qmp_add_debug_cfg_;
        case QMP_FLEXUS_SETDEBUG: return qmp_set_debug_;
        case QMP_FLEXUS_ENABLECATEGORY: return qmp_enable_category_;
        case QMP_FLEXUS_DISABLECATEGORY: return qmp_disable_category_;
        case QMP_FLEXUS_LISTCATEGORIES: return qmp_list_categories_;
        case QMP_FLEXUS_ENABLECOMPONENT: return qmp_enable_component_;
        case QMP_FLEXUS_DISABLECOMPONENT: return qmp_disable_component_;
        case QMP_FLEXUS_LISTCOMPONENTS: return qmp_list_components_;
        case QMP_FLEXUS_PRINTDEBUGCONFIGURATION: return qmp_print_debug_configuration_;
        case QMP_FLEXUS_WRITEDEBUGCONFIGURATION: return qmp_write_debug_configuration_;
        case QMP_FLEXUS_TERMINATESIMULATION: return qmp_terminate_simulation_;
        case QMP_FLEXUS_LOG: return qmp_log_;
        case QMP_FLEXUS_PRINTMMU: return qmp_print_mmu_;
        default: throw qmp_not_implemented();
    }
}

#endif // FLEXUS_QEMU_QMP_API_HPP_INCLUDED