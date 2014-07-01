#ifndef FLEXUS_EXCEPTION_HPP_INCLUDED
#define FLEXUS_EXCEPTION_HPP_INCLUDED

#include <string>
#include <cstring>
#include <memory>

namespace Flexus {
namespace Core {

using std::string;
using std::memcpy;
using std::strlen;
using std::exception;
using std::size_t;

#define FLEXUS_EXCEPTION(anExplanation)  FlexusException(__FILE__,__LINE__,anExplanation)
#define SIMICS_EXCEPTION(anExplanation)  SimicsException (__FILE__,__LINE__,anExplanation)

class FlexusException : public exception {
protected:
  mutable char * theExplanation; //Very ugly to make this mutable, but we need to have a
  //"transfer of ownership" copy constructor, and it needs to be usable with temporaries,
  //so the constructor must take a const reference, but needs to modify theExplanation.
  char const * theFile;
  int64_t theLine;
public:
  FlexusException()  : theExplanation(0), theFile(0), theLine(0) {}
  explicit FlexusException(string anExplanation) :
    theFile(0),
    theLine(0) {
    theExplanation = new char[anExplanation.size() +1];
    memcpy(theExplanation, anExplanation.c_str(), anExplanation.size() + 1);
  }
  explicit FlexusException(char const * anExplanation) :
    theFile(0),
    theLine(0) {
    size_t len = strlen(anExplanation);
    theExplanation = new char[len + 1];
    memcpy(theExplanation, anExplanation, len);
    theExplanation[len] = 0;
  }
  FlexusException(char const * aFile, int64_t aLine, string anExplanation) :
    theFile(aFile),
    theLine(aLine) {
    theExplanation = new char[anExplanation.size() +1];
    memcpy(theExplanation, anExplanation.c_str(), anExplanation.size() + 1);
  }
  FlexusException(char const * aFile, int64_t aLine, char const * anExplanation) :
    theFile(aFile),
    theLine(aLine) {
    size_t len = strlen(anExplanation);
    theExplanation = new char[len + 1];
    memcpy(theExplanation, anExplanation, len);
    theExplanation[len] = 0;
  }

  FlexusException(FlexusException const & anException) :
    theExplanation(anException.theExplanation),
    theFile(anException.theFile),
    theLine(anException.theLine) {
    //Copying an exception MOVES ownership of the explanation
    anException.theExplanation = 0;
  }

  virtual ~FlexusException() throw() {
    if (theExplanation) {
      delete [] theExplanation;
    }
  }

  virtual char const * no_explanation_str() const {
    return "Unknown Flexus exception";
  }
  virtual char const * what() const throw() {
    //Note: since only one FlexusException object can ever conceivably exist at a time, this
    //trick of using a static for returning the char const * is ok.  Note also that since calling
    //what() on an exception will almost always happen right before a call to exit(), it doesn't
    //matter how slow this code is.
    static std::string return_string;
    return_string.clear();
    if (theFile) {
      return_string += '[' ;
      return_string +=  theFile;
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

}//End Core

namespace Simics {

using std::string;
using std::memcpy;
using std::strlen;
using std::exception;
using std::size_t;

class SimicsException : public Flexus::Core::FlexusException {
  typedef Flexus::Core::FlexusException base;
public:
  SimicsException()  : base() {}
  explicit SimicsException(string anExplanation):
    base(anExplanation) {}
  explicit SimicsException(char const * anExplanation):
    base(anExplanation) {}
  SimicsException(char const * aFile, int64_t aLine, string anExplanation) :
    base(aFile, aLine, anExplanation) {}
  SimicsException(char const * aFile, int64_t aLine, char const * anExplanation) :
    base(aFile, aLine, anExplanation) {}
  virtual ~SimicsException() throw() {}
  virtual char const * no_explanation_str() const {
    return "Unknown Simics exception";
  }
};

}  //End Namespace Simics

namespace Qemu {
using std::string;
using std::memcpy;
using std::strlen;
using std::exception;
using std::size_t;

class QemuException : public Flexus::Core::FlexusException {
  typedef Flexus::Core::FlexusException base;
public:
  QemuException()  : base() {}
  explicit QemuException(string anExplanation):
    base(anExplanation) {}
  explicit QemuException(char const * anExplanation):
    base(anExplanation) {}
  QemuException(char const * aFile, int64_t aLine, string anExplanation) :
    base(aFile, aLine, anExplanation) {}
  QemuException(char const * aFile, int64_t aLine, char const * anExplanation) :
    base(aFile, aLine, anExplanation) {}
  virtual ~QemuException() throw() {}
  virtual char const * no_explanation_str() const {
    return "Unknown Simics exception";
  }
};

}
}
} //namespace Flexus

#endif //FLEXUS_EXCEPTION_HPP_INCLUDED

