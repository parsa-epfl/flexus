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

#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/none.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

void
connect(std::list<InternalDependance> const& dependances, simple_action& aSource)
{
    BaseSemanticAction& act = *(aSource.action);
    std::for_each(dependances.begin(),
                  dependances.end(),
                  boost::lambda::bind(&BaseSemanticAction::addDependance, ll::var(act), ll::_1));
}

void
BaseSemanticAction::addRef()
{
    /*
    ++theSemanticActionImbalance;
    if (theSemanticActionImbalance > theMaxSemanticActionImbalance  + 50) {
      theMaxSemanticActionImbalance = theSemanticActionImbalance;
      DBG_(Iface, ( << "Max outstanding semantic actions: " <<
    theSemanticActionImbalance) );
    }
    */
    boost::intrusive_ptr_add_ref(theInstruction);
}

void
BaseSemanticAction::releaseRef()
{
    //--theSemanticActionImbalance;
    boost::intrusive_ptr_release(theInstruction);
}

void
BaseSemanticAction::Dep::satisfy(int32_t anArg)
{
    SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction << " " << anArg);
    theAction.satisfy(anArg);
}
void
BaseSemanticAction::Dep::squash(int32_t anArg)
{
    SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction);
    theAction.squash(anArg);
}

void
BaseSemanticAction::satisfyDependants()
{
    if (!cancelled() && !signalled()) {
        for (int32_t i = 0; i < theEndOfDependances; ++i) {
            theDependances[i].satisfy();
        }
        theSignalled = true;
        theSquashed  = false;
    } else {
        SEMANTICS_DBG(theInstruction << "NOTE: Dependants were canceled!");
    }
}

void
BaseSemanticAction::satisfy(int32_t anArg)
{
    if (!cancelled()) {
        bool was_ready = ready();
        setReady(anArg, true);
        theSquashed = false;
        if (!was_ready && ready() && core()) {
            setReady(anArg, true);
            reschedule();
        }
    } else {
        SEMANTICS_DBG(theInstruction << "NOTE: Action were canceled!");
    }
}

void
BaseSemanticAction::squash(int32_t anOperand)
{
    SEMANTICS_DBG(*theInstruction << *this);
    setReady(anOperand, false);
    squashDependants();
}

void
BaseSemanticAction::squashDependants()
{
    if (!theSquashed) {
        if (core()) {

            SEMANTICS_DBG(theInstruction);
            for (int32_t i = 0; i < theEndOfDependances; ++i) {
                theDependances[i].squash();
            }
        }
        theSignalled = false;
        theSquashed  = true;
    }
}

void
BaseSemanticAction::evaluate()
{
    theScheduled = false;
    if (!cancelled()) { doEvaluate(); }
}

void
BaseSemanticAction::addDependance(InternalDependance const& aDependance)
{
    DBG_Assert(theEndOfDependances < 4);
    theDependances[theEndOfDependances] = aDependance;
    ++theEndOfDependances;
}

void
BaseSemanticAction::reschedule()
{
    if (!theScheduled && core()) {
        DBG_(Iface, (<< *this << " rescheduled"));
        theScheduled = true;
        core()->reschedule(this);
    }
}

void
connectDependance(InternalDependance const& aDependant, simple_action& aSource)
{
    aSource.action->addDependance(aDependant);
}

uArch*
BaseSemanticAction::core()
{
    return theInstruction->core();
}

PredicatedSemanticAction::PredicatedSemanticAction(SemanticInstruction* anInstruction,
                                                   int32_t aNumArgs,
                                                   bool anInitialPredicate)
  : BaseSemanticAction(anInstruction, aNumArgs)
  , thePredicate(anInitialPredicate)
  , thePredicateTarget(*this)
{
}

void
PredicatedSemanticAction::evaluate()
{
    theScheduled = false;
    if (thePredicate) { BaseSemanticAction::evaluate(); }
}

void
PredicatedSemanticAction::squash(int32_t anOperand)
{
    setReady(anOperand, false);
    if (thePredicate) { squashDependants(); }
}

void
PredicatedSemanticAction::predicate_off(int)
{
    if (!cancelled() && thePredicate) {
        SEMANTICS_DBG(*theInstruction << *this);
        reschedule();
        thePredicate = false;
        squashDependants();
    }
}

void
PredicatedSemanticAction::predicate_on(int)
{
    if (!cancelled() && !thePredicate) {
        SEMANTICS_DBG(*theInstruction << *this);
        reschedule();
        thePredicate = true;
        squashDependants();
    }
}

} // namespace nDecoder
