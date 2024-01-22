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
#if defined(DBG_NewCategories)
#error "DBG_NewCategories has been replaced with DBG_DefineCategories"
#endif

#if defined(DBG_SetCompileTimeMinSev)

#undef DBG__internal_tmp_MinimumSeverity
#define DBG__internal_tmp_MinimumSeverity                                                          \
  BOOST_PP_CAT(DBG__Undefined_Severity_Level__, DBG_SetCompileTimeMinSev)                          \
  (DBG_internal_Sev_to_int)(DBG_SetCompileTimeMinSev) /**/

#include <core/debug/aux_/clear_macros.hpp>

#include <core/debug/aux_/crit_macros.hpp>
#include <core/debug/aux_/dev_macros.hpp>
#include <core/debug/aux_/iface_macros.hpp>
#include <core/debug/aux_/inv_macros.hpp>
#include <core/debug/aux_/tmp_macros.hpp>
#include <core/debug/aux_/trace_macros.hpp>
#include <core/debug/aux_/verb_macros.hpp>
#include <core/debug/aux_/vverb_macros.hpp>

#undef DBG_SetCompileTimeMinSev

#endif // DBG_SetCompileTimeMinSev

#if defined(DBG_SetInitialRuntimeMinSev)
#ifdef DBG__internal_GlobalRunTimeMinSev_SET
#error "Runtime global minimum severity may only be set once"
#else
#define DBG__internal_GlobalRunTimeMinSev_SET
#endif

#include <core/debug/aux_/runtime_sev.hpp>

#undef DBG_SetInitialRuntimeMinSev

#endif // DBG_SetGlobalRunTimeMinSev

#if defined(DBG_DefineCategories)

#include <core/debug/aux_/new_categories.hpp>
#undef DBG_DefineCategories

#endif // DBG_NewCategories

#if defined(DBG_DeclareCategories)

#include <core/debug/aux_/declare_new_categories.hpp>
#undef DBG_DeclareCategories

#endif // DBG_DeclareCategories

#if defined(DBG_SetAssertions)

#include <core/debug/aux_/assert_macros.hpp>
#undef DBG_SetAssertions

#endif // DBG_SetMinimumSeverity

#if defined(DBG_SetDefaultOps)

#include <core/debug/aux_/default_ops.hpp>

#endif // DBG_SetDefaultOps

#if defined(DBG_Reset)

#undef DBG_Reset
#include <core/debug/aux_/reset.hpp>

#endif // DBG_SetDefaultOps
