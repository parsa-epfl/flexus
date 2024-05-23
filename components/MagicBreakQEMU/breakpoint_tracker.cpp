//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <core/debug/debug.hpp>
#include <core/qemu/api_wrappers.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <iostream>
#include <memory>
// #include <core/qemu/event_api.hpp>
#include <components/MagicBreakQEMU/breakpoint_tracker.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/component.hpp>
#include <core/flexus.hpp>
#include <core/qemu/configuration_api.hpp>

#define DBG_DefineCategories MagicBreak, IterationCount, Termination, IterationTrace, DBTransactionTrace, SimPrint
#define DBG_SetDefaultOps    AddCat(MagicBreak)
#include DBG_Control()

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::Qemu;

namespace nMagicBreak {

namespace Stat = Flexus::Stat;
// declare functions here
extern "C"
{
    void IterationTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint);
    void TransactionTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint);
    void BreakpointTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint);
    void RegressionTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aRegression);
    void SimPrintHandlerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint);
    void PacketTrackerEthernetFrame(void* obj, int32_t aNetworkID, int32_t aFrameType, long long aTimestamp);
    void ConsoleStringTrackerXTermString(void* obj, Qemu::API::conf_object_t* ignored, char* aString);
}

class IterationTrackerImpl : public IterationTracker
{
    static const int32_t kIterationCountBreakpoint = 4;
    std::vector<int32_t> theIterationCounts;
    int32_t theEndIteration;
    std::string theCurrentStatIteration;
    bool theCkptFlag;

  public:
    void OnMagicBreakpoint(Qemu::API::conf_object_t* aCpu, uint64_t aBreakpoint)
    {
        uint32_t cpu_no = Qemu::API::qemu_api.get_cpu_idx(aCpu);

        if (aBreakpoint == kIterationCountBreakpoint) {
            if (cpu_no >= theIterationCounts.size()) { theIterationCounts.resize(cpu_no + 1, 0); }
            ++theIterationCounts[cpu_no];
            DBG_(
              Dev,
              AddCat(IterationCount)(<< "CPU[" << cpu_no << "] has reached iteration " << theIterationCounts[cpu_no]));
            DBG_(Trace, Cat(IterationTrace) SetNumeric((Node)cpu_no) SetNumeric((Iteration)theIterationCounts[cpu_no]));

            if (cpu_no == 0) {
                Stat::getStatManager()->closeMeasurement(theCurrentStatIteration);
                theCurrentStatIteration =
                  std::string("Iteration ") + boost::padded_string_cast<3, '0'>(theIterationCounts[0]);
                Stat::getStatManager()->openMeasurement(theCurrentStatIteration);

                if (theEndIteration >= 0 && theIterationCounts[0] >= theEndIteration) {
                    DBG_(Dev,
                         AddCat(Termination)(<< "Simulation terminated because target iteration "
                                             << theIterationCounts[0] << " reached."));
                    Flexus::Core::theFlexus->terminateSimulation();
                    return;
                }

                if (theCkptFlag) { Flexus::Core::theFlexus->quiesceAndSave(theIterationCounts[0]); }
            }
        }
    }
    // Not sure if this goes here-- seems like wrong place
    // Qemu::API::QEMU_insert_callback(
    //		    Qemu::API::QEMU_callback_event_t = QEMU_magic_instruction
    ///		  , (void*) this
    //		  , (void*)&IterationTrackerMagicBreakpoint
    //		  );

  public:
    IterationTrackerImpl()
      : theEndIteration(-1)
      , theCkptFlag(false)
    {
    }

    void printIterationCounts(std::ostream& aStream)
    {
        for (uint32_t i = 0; i < theIterationCounts.size(); ++i) {
            aStream << "CPU[" << i << "]: " << theIterationCounts[i];
        }
    }
    void setIterationCount(uint32_t aCPU, int32_t aCount)
    {
        if (aCPU >= theIterationCounts.size()) { theIterationCounts.resize(aCPU + 1, 0); }
        theIterationCounts[aCPU] = aCount;
    }
    void enable()
    {
        //  Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicIterationTracker].obj = (void
        //  *)(this); Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicIterationTracker].fn  =
        //  (void *)(IterationTrackerMagicBreakpoint);

        int32_t iter = 0;
        if (theIterationCounts.size() > 0) { iter = theIterationCounts[0]; }
        theCurrentStatIteration = std::string("Iteration ") + boost::padded_string_cast<3, '0'>(iter);
        Stat::getStatManager()->openMeasurement(theCurrentStatIteration);
    }
    void enableCheckpoints() { theCkptFlag = true; }
    void endOnIteration(int32_t anIteration) { theEndIteration = anIteration; }
    void saveState(std::ostream& aStream)
    {
        aStream << theIterationCounts.size() << std::endl;
        for (uint32_t i = 0; i < theIterationCounts.size(); ++i) {
            aStream << theIterationCounts[i] << std::endl;
        }
    }
    void loadState(std::istream& aStream)
    {
        int32_t val = 0;
        aStream >> val;
        theIterationCounts.resize(val);
        for (int32_t i = 0; i < val; ++i) {
            aStream >> theIterationCounts[i];
        }
    }
};

class TransactionTrackerImpl : public BreakpointTracker
{

    Stat::StatCounter statDB2_Interval;
    Stat::StatCounter statDB2_NewOrder_Start;
    Stat::StatCounter statDB2_NewOrder_End;
    Stat::StatCounter statDB2_Payment_Start;
    Stat::StatCounter statDB2_Payment_End;
    Stat::StatCounter statDB2_OrdStat_Start;
    Stat::StatCounter statDB2_OrdStat_End;
    Stat::StatCounter statDB2_Delivery_Start;
    Stat::StatCounter statDB2_Delivery_End;
    Stat::StatCounter statDB2_StockLev_Start;
    Stat::StatCounter statDB2_StockLev_End;

    Stat::StatCounter statJBB_MultiOrder_Start;
    Stat::StatCounter statJBB_MultiOrder_End;
    Stat::StatCounter statJBB_Other;

    Stat::StatCounter statWEB_Class0;
    Stat::StatCounter statWEB_Class1;
    Stat::StatCounter statWEB_Class2;
    Stat::StatCounter statWEB_Class3;

    int32_t theTransactionType;
    int32_t theStopTransaction;
    int32_t theStatInterval;
    int32_t theCkptInterval;
    int32_t theCurrentStatInterval;
    std::string theCurrentStatIntervalName;
    int32_t theTransactionCount;
    int32_t theLastIntervalCount;
    int32_t theLastCkptCount;
    uint64_t theCycleMinimum;

  public:
    void OnMagicBreakpoint(Qemu::API::conf_object_t* aCpu, uint64_t aBreakpoint)
    {

        if (theTransactionType == 0) {
            doTpccJbbTransaction(aBreakpoint);
        } else if (theTransactionType == 1) {
            doWebTransaction(aBreakpoint);
        } else {
            DBG_Assert(false, (<< "unknown transaction type: " << theTransactionType));
        }

        if (theStatInterval > 0 && (theTransactionCount - theLastIntervalCount) > theStatInterval) {
            Stat::getStatManager()->closeMeasurement(theCurrentStatIntervalName);
            theCurrentStatIntervalName =
              std::string("Trans Interval ") + boost::padded_string_cast<3, '0'>(theCurrentStatInterval++);
            Stat::getStatManager()->openMeasurement(theCurrentStatIntervalName);

            theLastIntervalCount = theTransactionCount;
        }

        if (theStopTransaction >= 0 && theTransactionCount >= theStopTransaction) {
            if (Flexus::Core::theFlexus->cycleCount() > theCycleMinimum) {
                DBG_(Dev,
                     (<< "Reached target transaction and minimum cycle count. "
                         "Ending simulation."));
                Flexus::Core::theFlexus->terminateSimulation();
            }
        }

        if (theCkptInterval > 0 && (theTransactionCount - theLastCkptCount) >= theCkptInterval) {
            DBG_(Dev, (<< "Reached transaction " << theTransactionCount << ". Writing Checkpoint."));
            Flexus::Core::theFlexus->quiesceAndSave(theTransactionCount);
            theLastCkptCount = theTransactionCount;
        }
    }

    void doTpccJbbTransaction(int64_t aBreakpoint)
    {
        switch (aBreakpoint) {
            case 2:
                ++statDB2_Interval;
                DBG_(Iface, (<< "Start of Measurement Interval"));
                break;
            case 3:
                ++statDB2_NewOrder_Start;
                DBG_(Iface, (<< "Start NewOrder"));
                break;
            case 30:
                ++statDB2_NewOrder_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "NewOrder"));
                DBG_(Iface, (<< "End NewOrder"));
                break;
            case 4:
                ++statDB2_Payment_Start;
                DBG_(Iface, (<< "Start Payment"));
                break;
            case 40:
                ++statDB2_Payment_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "Payment"));
                DBG_(Iface, (<< "End Payment"));
                break;
            case 5:
                ++statDB2_OrdStat_Start;
                DBG_(Iface, (<< "Start OrdStat"));
                break;
            case 50:
                ++statDB2_OrdStat_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "OrdStat"));
                DBG_(Iface, (<< "End OrdStat"));
                break;
            case 6:
                ++statDB2_Delivery_Start;
                DBG_(Iface, (<< "Start Delivery"));
                break;
            case 60:
                ++statDB2_Delivery_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "Delivery"));
                DBG_(Iface, (<< "End Delivery"));
                break;
            case 7:
                ++statDB2_StockLev_Start;
                DBG_(Iface, (<< "Start StockLev"));
                break;
            case 70:
                ++statDB2_StockLev_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "StockLev"));
                DBG_(Iface, (<< "End StockLev"));
                break;
            case 8:
                ++statJBB_MultiOrder_Start;
                DBG_(Iface, (<< "Start MultiOrder"));
                break;
            case 80:
                ++statJBB_MultiOrder_End;
                ++theTransactionCount;
                DBG_(Trace, Cat(DBTransactionTrace) Set((Type) << "MultiOrder"));
                DBG_(Iface, (<< "End MultiOrder"));
                break;
            default:
                ++statJBB_Other;
                DBG_(Iface, (<< "Unknown Breakpoint"));
                break;
        }
    }

    void doWebTransaction(int64_t aBreakpoint)
    {
        switch (aBreakpoint) {
            case 2:
                ++statDB2_Interval;
                DBG_(Iface, (<< "Start of Measurement Interval"));
                break;
            case 3:
                ++statWEB_Class0;
                ++theTransactionCount;
                DBG_(Iface, (<< "Class 0 transaction"));
                break;
            case 4:
                ++statWEB_Class1;
                ++theTransactionCount;
                DBG_(Iface, (<< "Class 1 transaction"));
                break;
            case 5:
                ++statWEB_Class2;
                ++theTransactionCount;
                DBG_(Iface, (<< "Class 2 transaction"));
                break;
            case 6:
                ++statWEB_Class3;
                ++theTransactionCount;
                DBG_(Iface, (<< "Class 3 transaction"));
                break;
            default:
                ++statJBB_Other;
                DBG_(Iface, (<< "Unknown Breakpoint"));
                break;
        }
    }
    // Don't think it should be here
    //  Qemu::API::QEMU_insert_callback(
    //		    Qemu::API::QEMU_magic_instruction
    //		  , (void*) this
    //		  , (void*)&TransactionTrackerOnMagicBreakpoint
    //		  );

  public:
    TransactionTrackerImpl(int32_t aTransactionType,
                           int32_t aStopTransaction,
                           int32_t aStatInterval,
                           int32_t aCkptInterval,
                           int32_t aFirstTransactionIs,
                           uint64_t aCycleMinimum)
      : statDB2_Interval("DB2 Interval")
      , statDB2_NewOrder_Start("DB2 NewOrder(begin)")
      , statDB2_NewOrder_End("DB2 NewOrder(commit)")
      , statDB2_Payment_Start("DB2 Payment(begin)")
      , statDB2_Payment_End("DB2 Payment(commit)")
      , statDB2_OrdStat_Start("DB2 OrdStat(begin)")
      , statDB2_OrdStat_End("DB2 OrdStat(commit)")
      , statDB2_Delivery_Start("DB2 Delivery(begin)")
      , statDB2_Delivery_End("DB2 Delivery(commit)")
      , statDB2_StockLev_Start("DB2 StockLev(begin)")
      , statDB2_StockLev_End("DB2 StockLev(commit)")
      , statJBB_MultiOrder_Start("JBB MultiOrder(begin)")
      , statJBB_MultiOrder_End("JBB MultiOrder(commit)")
      , statJBB_Other("JBB Other Magic Break")
      , statWEB_Class0("WEB Class 0")
      , statWEB_Class1("WEB Class 1")
      , statWEB_Class2("WEB Class 2")
      , statWEB_Class3("WEB Class 3")
      , theTransactionType(aTransactionType)
      , theStopTransaction(aStopTransaction + aFirstTransactionIs)
      , theStatInterval(aStatInterval)
      , theCkptInterval(aCkptInterval)
      , theCurrentStatInterval((theStatInterval == 0) ? 0 : (aFirstTransactionIs / theStatInterval))
      , theCurrentStatIntervalName("disabled")
      , theTransactionCount(aFirstTransactionIs)
      , theLastIntervalCount(aFirstTransactionIs)
      , theLastCkptCount(aFirstTransactionIs)
      , theCycleMinimum(aCycleMinimum)
    {
        if (theStatInterval > 0) {
            theCurrentStatIntervalName =
              std::string("Trans Interval ") + boost::padded_string_cast<3, '0'>(theCurrentStatInterval++);
            Stat::getStatManager()->openMeasurement(theCurrentStatIntervalName);
        }

        // not sure it goes here
        //  Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicTransactionTracker].obj = (void
        //  *)(this); Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicTransactionTracker].fn
        //  = (void *)(TransactionTrackerMagicBreakpoint);
    }
};

class TerminateOnMagicBreakTracker : public BreakpointTracker
{
    int32_t theMagicBreakpoint;

  public:
    void OnMagicBreakpoint(Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {

        if (aBreakpoint == theMagicBreakpoint) {
            DBG_(
              Dev,
              AddCat(Termination)(<< "Simulation terminated because magic breakpont " << aBreakpoint << " reached."));

            Flexus::Core::theFlexus->terminateSimulation();
        }
    }

  public:
    TerminateOnMagicBreakTracker(int32_t aBreakpoint)
      : theMagicBreakpoint(aBreakpoint)
    {
        //  Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicBreakpointTracker].obj = (void
        //  *)(this); Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicBreakpointTracker].fn  =
        //  (void *)(BreakpointTrackerMagicBreakpoint);
    }
};

class RegressionTrackerImpl : public RegressionTracker
{
  private:
    int system_width;
    std::vector<int> struct_id;

  public:
    void OnMagicBreakpoint(Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {
        DBG_(Dev, (<< "Regression Testing Breakpoint: " << aBreakpoint));
        if (aBreakpoint == theStopBreakpoint) {
            DBG_(Dev, (<< "Stop breakpoint.  Terminating Simulation."));
            Flexus::Core::theFlexus->terminateSimulation();
        }
        theLastBreakpoint = aBreakpoint;
    }

    int64_t theLastBreakpoint;
    int64_t theStopBreakpoint;

  public:
    RegressionTrackerImpl()
      : system_width(ComponentManager::getComponentManager().systemWidth())
      , struct_id(system_width)
      , theLastBreakpoint(0)
      , theStopBreakpoint(1)
    {
        //  Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicRegressionTracker].obj = (void *)
        //  this; Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicRegressionTracker].fn =
        //  (void *) &RegressionTrackerMagicBreakpoint;
    }

    void enable()
    {
        DBG_(Dev, (<< "Regression Testing Mode Enabled."));
        // theMagicBreakpointHap.reset(new on_magic_break_t(this));
    }
};

class CycleTrackerImpl : public CycleTracker
{
    uint64_t theStopCycle;
    uint64_t theCkptInterval;
    uint64_t theLastCkpt;

  public:
    CycleTrackerImpl(uint64_t aStopCycle, uint64_t aCkptInterval, uint32_t aCkptNameStart)
      : theStopCycle(aStopCycle)
      , theCkptInterval(aCkptInterval)
      , theLastCkpt(0)
    {
        //((FlexusImpl *)(Flexus::Core::theFlexus))->setStopCycle(aStopCycle);
    }

    virtual void tick()
    {
        if (theCkptInterval > 0 && Flexus::Core::theFlexus->cycleCount() > theLastCkpt + theCkptInterval) {
            DBG_(Dev,
                 (<< "Reached target transaction and minimum cycle count. "
                     "Ending simulation."));
            Flexus::Core::theFlexus->quiesceAndSave();
            theLastCkpt = Flexus::Core::theFlexus->cycleCount();
        }

        if (theStopCycle > 0 && Flexus::Core::theFlexus->cycleCount() >= theStopCycle) {
            DBG_(Dev, (<< "Reached target cycle. Ending simulation."));
            Flexus::Core::theFlexus->terminateSimulation();
        }
    }
};

struct xact_version1
{
    uint64_t struct_version;
    uint64_t pid;
    uint64_t xact_num;
    uint64_t xact_type;
    uint64_t marker_type;
    uint64_t marker_num;
    uint64_t xact_struct_base_addr;
    uint64_t canary;
};

struct web_version1
{
    uint64_t struct_version;
    uint64_t client;
    uint64_t generator;
    uint64_t marker_type;
    uint64_t marker_num;
    uint64_t curr_time;
    uint64_t type_or_size;
    uint64_t class_or_sleep;
    uint64_t base_addr;
    uint64_t canary;
};

using Flexus::SharedTypes::VirtualMemoryAddress;
// // Helperfunction to read Vadddresses
// char readVirtualAddress(Qemu::API::conf_object_t *cpu, VirtualMemoryAddress anAddr, int size) {

//   uint64_t addr = Qemu::API::qemu_api.translate_va2pa(cpu, Qemu::API::QEMU_DI_Data, anAddr);
//   uint8_t *buf = new uint8_t[size];
//   Qemu::API::qemu_api.get_mem(buf, addr, size);
//   char ret = buf[0];
//   delete[] buf;
//   return ret;
// }

// uint64_t readG(Qemu::API::conf_object_t *cpu, int reg) {
//   // XXXX
//   return 0;
// //return Qemu::API::QEMU_read_register(cpu, Qemu::API::kGENERAL, reg);
// }

class SimPrintHandlerImpl : public SimPrintHandler
{

    char const* marker(xact_version1& xact)
    {
        switch (xact.marker_type) {
            case 1: return "start";
            case 2: return "successful";
            case 3: return "failed";
            case 4:
                switch (xact.xact_type) {
                    case 1: // NewOrder
                        switch (xact.marker_num) {
                            case 1: return "start-attempt";
                            case 2: return "mark-2";
                            case 3: return "mark-3";
                            case 4: return "mark-4";
                            case 5: return "mark-5";
                            case 6: return "mark-6";
                            case 7: return "mark-7";
                            case 8: return "mark-8";
                            case 9: return "mark-9";
                            case 10: return "mark-10";
                            case 11: return "commit";
                            case 12: return "rollback";
                            default: return "unknown";
                        }
                    case 2: // Payment
                        switch (xact.marker_num) {
                            case 1: return "start-attempt";
                            case 2: return "mark-2";
                            case 3: return "mark-3";
                            case 4: return "mark-4";
                            case 5: return "mark-5";
                            case 6: return "mark-6";
                            case 7: return "commit";
                            case 8: return "rollback";
                            default: return "unknown";
                        }
                    case 3: // OrdStat
                        switch (xact.marker_num) {
                            case 1: return "start-attempt";
                            case 2: return "mark-2";
                            case 3: return "mark-3";
                            case 4: return "mark-4";
                            case 5: return "mark-5";
                            case 6: return "commit";
                            case 7: return "rollback";
                            default: return "unknown";
                        }
                    case 4: // Delivery
                        switch (xact.marker_num) {
                            case 1: return "start-attempt";
                            case 2: return "mark-2";
                            case 3: return "mark-3";
                            case 4: return "mark-4";
                            case 5: return "commit";
                            case 6: return "rollback";
                            default: return "unknown";
                        }
                    case 5: // StockLev
                        switch (xact.marker_num) {
                            case 1: return "start-attempt";
                            case 2: return "mark-2";
                            case 3: return "commit";
                            case 4: return "rollback";
                            default: return "unknown";
                        }
                    default: return "Unknown";
                }
            default: return "unknown";
        }
    }

    char const* xactType(xact_version1& xact)
    {
        switch (xact.xact_type) {
            case 1: return "NewOrder";
            case 2: return "Payment";
            case 3: return "OrdStat";
            case 4: return "Delivery";
            case 5: return "StockLev";
            default: return "Unknown";
        }
    }

  public:
    void OnMagicBreakpoint(Qemu::API::conf_object_t* aCpu, long long aBreakpoint) {}

    int64_t theLastBreakpoint;
    int64_t theStopBreakpoint;

  public:
    SimPrintHandlerImpl()
    {
        //  Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicSimPrintHandler].obj = (void
        //  *)(this); Flexus::Qemu::API::qflex_sim_callbacks.magic_inst[Flexus::Qemu::API::MagicSimPrintHandler].fn  =
        //  (void *)(SimPrintHandlerMagicBreakpoint);
    }
};

class PacketTrackerImpl : public BreakpointTracker
{

    Qemu::API::conf_object_t* theNetwork;

    Stat::StatCounter thePackets;
    Stat::StatCounter thePackets_ClientToServer;
    Stat::StatCounter thePackets_ServerToClient;
    Stat::StatCounter theServerTxData;

    void OnPacketV2(Qemu::API::conf_object_t* aNetwork, long long aTimestamp) { OnPacket(0, 0, aTimestamp); }

  public:
    void OnPacket(int32_t aNetworkID, int32_t aFrameType, long long aTimestamp)
    {
        if (theNetwork == 0) { return; }
        DBG_(Dev, (<< "Packet tracing is currently unsupported in QEMU."));
    }

  public:
    PacketTrackerImpl(int32_t aSrcPortNumber, char aServerMACCode, char aClientMACCode)
      : theNetwork(0)
      , thePackets("sys-Packets")
      , thePackets_ClientToServer("sys-Packets:C2S")
      , thePackets_ServerToClient("sys-Packets:S2C")
      , theServerTxData("sys-ServerTxData")
    {
        // Not supported
        theNetwork = NULL;
        /*
        if (theNetwork != 0) {
          Flexus::Qemu::API::qflex_sim_callbacks.ethernet_frame.obj = (void *)(this);
          Flexus::Qemu::API::qflex_sim_callbacks.ethernet_frame.fn  = (void *)(PacketTrackerEthernetFrame);
        }
        */
    }
};

class ConsoleStringTrackerImpl : public ConsoleStringTracker
{
  public:
    void OnXtermString(Qemu::API::conf_object_t* ignored, char* aString)
    {
        DBG_(Dev, (<< "Console termination string " << aString << " has appeared."));
        Flexus::Core::theFlexus->terminateSimulation();
    }

  public:
    void addString(std::string const& aString) {}

    ConsoleStringTrackerImpl()
    {
        //  Flexus::Qemu::API::qflex_sim_callbacks.xterm_break_string.obj = (void *)(this);
        //  Flexus::Qemu::API::qflex_sim_callbacks.xterm_break_string.fn  = (void *)(&ConsoleStringTrackerXTermString);
    }
};

extern "C"
{
    void SimPrintHandlerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {
        static_cast<class SimPrintHandlerImpl*>(obj)->OnMagicBreakpoint(aCpu, aBreakpoint);
    }

    void IterationTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {
        static_cast<class IterationTrackerImpl*>(obj)->OnMagicBreakpoint(aCpu, aBreakpoint);
    }

    void TransactionTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {
        static_cast<class TransactionTrackerImpl*>(obj)->OnMagicBreakpoint(aCpu, aBreakpoint);
    }

    void BreakpointTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aBreakpoint)
    {
        static_cast<TerminateOnMagicBreakTracker*>(obj)->OnMagicBreakpoint(aCpu, aBreakpoint);
    }

    void RegressionTrackerMagicBreakpoint(void* obj, Qemu::API::conf_object_t* aCpu, long long aRegression)
    {
        // static_cast<RegressionTrackerImpl *>(obj)->OnMagicBreakpoint(aCpu, aRegression);
    }

    void ConsoleStringTrackerXTermString(void* obj, Qemu::API::conf_object_t* ignored, char* aString)
    {
        static_cast<ConsoleStringTrackerImpl*>(obj)->OnXtermString(ignored, aString);
    }

    void PacketTrackerEthernetFrame(void* obj, int32_t aNetworkID, int32_t aFrameType, long long aTimestamp)
    {
        static_cast<PacketTrackerImpl*>(obj)->OnPacket(aNetworkID, aFrameType, aTimestamp);
    }
}

// FIXME Possibly incorrect in some way, was commented
boost::intrusive_ptr<IterationTracker>
BreakpointTracker::newIterationTracker()
{
    return new IterationTrackerImpl();
}
boost::intrusive_ptr<BreakpointTracker>
BreakpointTracker::newTransactionTracker(int32_t aTransactionType,
                                         int32_t aStopTransaction,
                                         int32_t aStatInterval,
                                         int32_t aCkptInterval,
                                         int32_t aFirstTransactionIs,
                                         uint64_t aMinCycles)
{
    return new TransactionTrackerImpl(aTransactionType,
                                      aStopTransaction,
                                      aStatInterval,
                                      aCkptInterval,
                                      aFirstTransactionIs,
                                      aMinCycles);
}
boost::intrusive_ptr<BreakpointTracker>
BreakpointTracker::newTerminateOnMagicBreak(int32_t aBreakpoint)
{
    return new TerminateOnMagicBreakTracker(aBreakpoint);
}
boost::intrusive_ptr<RegressionTracker>
BreakpointTracker::newRegressionTracker()
{
    return new RegressionTrackerImpl();
}
boost::intrusive_ptr<BreakpointTracker>
BreakpointTracker::newSimPrintHandler()
{
    return new SimPrintHandlerImpl();
}
boost::intrusive_ptr<CycleTracker>
BreakpointTracker::newCycleTracker(uint64_t aStopCycle, uint64_t aCkptInterval, uint32_t aCkptNameStart)
{
    return new CycleTrackerImpl(aStopCycle, aCkptInterval, aCkptNameStart);
}
boost::intrusive_ptr<BreakpointTracker>
BreakpointTracker::newPacketTracker(int32_t aSrcPortNumber, char aServerMACCode, char aClientMACCode)
{
    return new PacketTrackerImpl(aSrcPortNumber, aServerMACCode, aClientMACCode);
}
boost::intrusive_ptr<ConsoleStringTracker>
BreakpointTracker::newConsoleStringTracker()
{
    return new ConsoleStringTrackerImpl();
}

} // namespace nMagicBreak
