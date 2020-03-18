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
#ifndef FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <core/debug/entry.hpp>
#include <core/debug/format.hpp>

namespace Flexus {
namespace Dbg {

class Action {
public:
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent) = 0;
  virtual void process(Entry const &anEntry) = 0;
  virtual ~Action() {
  }
};

class CompoundAction : public Action {
  std::vector<Action *> theActions; // These actions are owned by the pointers in theActions
public:
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
  virtual ~CompoundAction();
  void add(std::unique_ptr<Action> anAction);
};

class ConsoleLogAction : public Action {
  std::unique_ptr<Format> theFormat;

public:
  ConsoleLogAction(Format *aFormat) : theFormat(aFormat){};
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

class FileLogAction : public Action {
  std::string theFilename;
  std::ostream &theOstream;
  std::unique_ptr<Format> theFormat;

public:
  FileLogAction(std::string aFilename, Format *aFormat);
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

struct AbortAction : public Action {
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

struct BreakAction : public Action {
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

struct PrintStatsAction : public Action {
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

struct SeverityAction : public Action {
  uint32_t theSeverity;

public:
  SeverityAction(uint32_t aSeverity);
  virtual void printConfiguration(std::ostream &anOstream, std::string const &anIndent);
  virtual void process(Entry const &anEntry);
};

#if 0
class SaveAction : public Action {
  std::string theBufferName;
  uint32_t theSize;
public:
  SaveAction(std::string aBufferName, uint32_t aCircularBufferSize);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
  virtual void process(Entry const & anEntry);
};

class FileSpillAction : public Action {
  std::string theBufferName;
  std::string theFilename;
  std::ostream & theOstream;
  std::unique_ptr<Format> theFormat;
public:
  FileSpillAction(std::string const & aBufferName, std::string const & aFilename, Format * aFormat);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
  virtual void process(Entry const & anEntry);
};

class ConsoleSpillAction : public Action {
  std::string theBufferName;
  std::unique_ptr<Format> theFormat;
public:
  ConsoleSpillAction(std::string const & aBufferName, Format * aFormat);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
  virtual void process(Entry const & anEntry);
};
#endif

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED
