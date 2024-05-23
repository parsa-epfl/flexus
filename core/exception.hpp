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
#ifndef FLEXUS_EXCEPTION_HPP_INCLUDED
#define FLEXUS_EXCEPTION_HPP_INCLUDED

#include <cstring>
#include <memory>
#include <string>

namespace Flexus {
namespace Core {

using std::exception;
using std::memcpy;
using std::size_t;
using std::string;
using std::strlen;

#define FLEXUS_EXCEPTION(anExplanation) FlexusException(__FILE__, __LINE__, anExplanation)
#define SIMICS_EXCEPTION(anExplanation) SimicsException(__FILE__, __LINE__, anExplanation)

class FlexusException : public exception
{
  protected:
    mutable char* theExplanation; // Very ugly to make this mutable, but we need to have a
    //"transfer of ownership" copy constructor, and it needs to be usable with
    // temporaries, so the constructor must take a const reference, but needs to
    // modify theExplanation.
    char const* theFile;
    int64_t theLine;

  public:
    FlexusException()
      : theExplanation(nullptr)
      , theFile(nullptr)
      , theLine(0)
    {
    }
    explicit FlexusException(string anExplanation)
      : theFile(nullptr)
      , theLine(0)
    {
        theExplanation = new char[anExplanation.size() + 1];
        memcpy(theExplanation, anExplanation.c_str(), anExplanation.size() + 1);
    }
    explicit FlexusException(char const* anExplanation)
      : theFile(nullptr)
      , theLine(0)
    {
        size_t len     = strlen(anExplanation);
        theExplanation = new char[len + 1];
        memcpy(theExplanation, anExplanation, len);
        theExplanation[len] = 0;
    }
    FlexusException(char const* aFile, int64_t aLine, string anExplanation)
      : theFile(aFile)
      , theLine(aLine)
    {
        theExplanation = new char[anExplanation.size() + 1];
        memcpy(theExplanation, anExplanation.c_str(), anExplanation.size() + 1);
    }
    FlexusException(char const* aFile, int64_t aLine, char const* anExplanation)
      : theFile(aFile)
      , theLine(aLine)
    {
        size_t len     = strlen(anExplanation);
        theExplanation = new char[len + 1];
        memcpy(theExplanation, anExplanation, len);
        theExplanation[len] = 0;
    }

    FlexusException(FlexusException const& anException)
      : theExplanation(anException.theExplanation)
      , theFile(anException.theFile)
      , theLine(anException.theLine)
    {
        // Copying an exception MOVES ownership of the explanation
        anException.theExplanation = nullptr;
    }

    virtual ~FlexusException() throw()
    {
        if (theExplanation) { delete[] theExplanation; }
    }

    virtual char const* no_explanation_str() const { return "Unknown Flexus exception"; }
    virtual char const* what() const throw()
    {
        // Note: since only one FlexusException object can ever conceivably exist at
        // a time, this trick of using a static for returning the char const * is
        // ok. Note also that since calling what() on an exception will almost
        // always happen right before a call to exit(), it doesn't matter how slow
        // this code is.
        static std::string return_string;
        return_string.clear();
        if (theFile) {
            return_string += '[';
            return_string += theFile;
            return_string += ':';
            return_string += theLine;
            return_string += "] ";
        }
        if (theExplanation) {
            return_string += theExplanation;
        } else {
            return_string += no_explanation_str();
        }
        return return_string.c_str();
    }
};

} // namespace Core

namespace Simics {

using std::exception;
using std::memcpy;
using std::size_t;
using std::string;
using std::strlen;

class SimicsException : public Flexus::Core::FlexusException
{
    typedef Flexus::Core::FlexusException base;

  public:
    SimicsException()
      : base()
    {
    }
    explicit SimicsException(string anExplanation)
      : base(anExplanation)
    {
    }
    explicit SimicsException(char const* anExplanation)
      : base(anExplanation)
    {
    }
    SimicsException(char const* aFile, int64_t aLine, string anExplanation)
      : base(aFile, aLine, anExplanation)
    {
    }
    SimicsException(char const* aFile, int64_t aLine, char const* anExplanation)
      : base(aFile, aLine, anExplanation)
    {
    }
    virtual ~SimicsException() throw() {}
    virtual char const* no_explanation_str() const { return "Unknown Simics exception"; }
};

} // End Namespace Simics

namespace Qemu {
using std::exception;
using std::memcpy;
using std::size_t;
using std::string;
using std::strlen;

class QemuException : public Flexus::Core::FlexusException
{
    typedef Flexus::Core::FlexusException base;

  public:
    QemuException()
      : base()
    {
    }
    explicit QemuException(string anExplanation)
      : base(anExplanation)
    {
    }
    explicit QemuException(char const* anExplanation)
      : base(anExplanation)
    {
    }
    QemuException(char const* aFile, int64_t aLine, string anExplanation)
      : base(aFile, aLine, anExplanation)
    {
    }
    QemuException(char const* aFile, int64_t aLine, char const* anExplanation)
      : base(aFile, aLine, anExplanation)
    {
    }
    virtual ~QemuException() throw() {}
    virtual char const* no_explanation_str() const { return "Unknown QEMU exception"; }
};

} // namespace Qemu
} // namespace Flexus

#endif // FLEXUS_EXCEPTION_HPP_INCLUDED
