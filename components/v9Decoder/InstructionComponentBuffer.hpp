#ifndef FLEXUS_v9DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED
#define FLEXUS_v9DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED

#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>
#include <core/stats.hpp>

static const size_t kICBSize = 2048;

namespace nv9Decoder {
extern Flexus::Stat::StatCounter theICBs;

struct InstructionComponentBuffer {

  char theComponentBuffer[kICBSize ];
  char * theNextAlloc;
  size_t theFreeSpace;
  InstructionComponentBuffer * theExtensionBuffer;

  InstructionComponentBuffer()
    : theNextAlloc(theComponentBuffer)
    , theFreeSpace(kICBSize)
    , theExtensionBuffer(0) {
    ++theICBs;
  }

  ~InstructionComponentBuffer() {
    if (theExtensionBuffer) {
      delete theExtensionBuffer;
      theExtensionBuffer = 0;
    }
    --theICBs;
  }

  void * alloc(size_t aSize) {
    DBG_Assert( aSize < kICBSize );
    if ( theFreeSpace < aSize ) {
      if (theExtensionBuffer == 0) {
        theExtensionBuffer = new InstructionComponentBuffer();
      }
      return theExtensionBuffer->alloc( aSize );
    }
    void * tmp = theNextAlloc;
    theNextAlloc += aSize;
    theFreeSpace -= aSize;
    DBG_Assert(theFreeSpace >= 0);
    return tmp;
  }
};

struct UncountedComponent {
  void operator delete(void *) {
    /*Nothing to do on delete*/
  }
  void * operator new( size_t aSize, InstructionComponentBuffer & aICB ) {
    return aICB.alloc( aSize );
  }

private:
  void * operator new( size_t ); //Not allowed

};

} //nv9Decoder

#endif //FLEXUS_v9DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED

