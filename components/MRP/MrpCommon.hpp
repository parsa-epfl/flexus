namespace nMrpTable {

inline uint32_t log_base2(uint32_t num) {
  uint32_t ii = 0;
  while (num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

inline uint32_t max(uint32_t a, uint32_t b) {
  return (a > b ? a : b);
}

struct ReadersConf {
  // the lower 16 bits contain the MSB of the counter for each node -
  // since we predict on (binary) values 10 and 11, these lower 16 bits
  // are the prediction
  ReadersConf()
    : theReaders(0)
  {}
  ReadersConf(uint32_t readers)
    : theReaders(0) {
    setReaders(readers);
  }

  const uint32_t predict() const {
    return (theReaders & 0xffff);
  }
  void setReaders(uint32_t readers) {
    // to initialize values to 10 (i.e. predict by default), copy the values
    // into the lower 16 bits
    // to initialize values to 01 (i.e. not predict by default), copy the values
    // into the upper 16 bits
    theReaders = (readers << 16) & 0xffff0000;
  }
  void updateReaders(uint32_t actual) {
    uint32_t newConf = 0;
    // increase the confidence of every actual reader and decrease confidence
    // of the others
    for (int32_t ii = 15; ii >= 0; ii--) {
      uint32_t conf = (((theReaders >> ii) & 0x1) << 1) | ((theReaders >> (ii + 16)) & 0x1);
      if (actual & (1 << ii)) {
        // this node actually read => increase confidence
        if (conf < 3) {
          conf++;
        }
      } else {
        // this node didn't read => decrease confidence
        if (conf > 0) {
          conf--;
        }
      }
      if (conf & 0x2) { // MSB
        newConf |= (1 << ii);
      }
      if (conf & 0x1) { // LSB
        newConf |= (1 << (ii + 16));
      }
    }
    DBG_(VVerb, Condition(theReaders != newConf)
         ( << "update conf: old=" << std::hex << theReaders
           << " actual=" << actual << " new=" << newConf ) );
    theReaders = newConf;
  }

  friend std::ostream & operator << (std::ostream & anOstream, ReadersConf const & aConf) {
    anOstream << "/" << & std::hex << aConf.predict() << "/ ^" << aConf.theReaders << & std::dec << "^";
    return anOstream;
  }

  uint32_t theReaders;
};  // class ReadersConf

struct ReadersNoConf {
  ReadersNoConf()
    : theReaders(0)
  {}
  ReadersNoConf(uint32_t readers)
    : theReaders(readers)
  {}

  const uint32_t predict() {
    return theReaders;
  }
  void setReaders(uint32_t readers) {
    theReaders = (readers & 0xffff);
  }
  void updateReaders(uint32_t actual) {
    // next time, use the superset of both the previously predicted readers
    // and the actual readers
    theReaders |= (actual & 0xffff);
  }

  uint32_t theReaders;
};  // class ReadersNoConf

struct GniadyConf {
  GniadyConf()
    : theReaders(0)
    , theConf(0)
  {}
  GniadyConf(uint32_t readers)
    : theReaders(readers)
    , theConf(/*theFastWarming ? 2 :*/ 1)
  {}

  const uint32_t predict() {
    uint32_t prediction = 0;
    if (theConf >= 2) {
      prediction = theReaders;
    }
    theLastPredict = theReaders;
    return prediction;
  }
  void setReaders(uint32_t readers) {
    theReaders = (readers & 0xffff);
  }
  void updateReaders(uint32_t actual) {
    //See if we should increase or decrease confidence
    int32_t correctly_predicted = actual & theLastPredict;
    if (correctly_predicted) {
      if ( static_cast<float>(count(correctly_predicted)) / count(actual) >= 0.8 ) {
        //>= 80% accuracy on last prediction
        if (theConf != 3) {
          ++theConf;
        }
      } else {
        //< 80% accuracy on last prediction
        if (theConf != 0) {
          --theConf;
        }
      }
    } else {
      //0% accuracy on last prediction
      if (theConf != 0) {
        --theConf;
      }
    }

    //Next time, use the list of readers from this time
    theReaders = (actual & 0xffff);
  }

  int32_t count(uint32_t value) {
    int32_t ret = 0;
    uint32_t sharers = value;
    for (int32_t ii = 0; ii < 16; ii++) {
      if (sharers & 0x01) {
        ret++;
      }
      sharers >>= 1;
    }
    return ret;
  }

  friend std::ostream & operator << (std::ostream & anOstream, GniadyConf const & aConf) {
    anOstream << & std::hex << "/" << aConf.theLastPredict << "/ \\" << aConf.theReaders << & std::dec << "\\ ^" << aConf.theConf << "^";
    return anOstream;
  }

  uint32_t theReaders;
  uint32_t theLastPredict;
  uint32_t theConf;
};  // class ReadersNoConf

struct HistoryEvent {
  HistoryEvent()
    : value(0)
  {}
  HistoryEvent(uint32_t readers)
    : value(readers)
  {}

  void set(bool write, uint32_t node) {
    if (write) {
      value = (1 << 16) | (node & 0xf);
    } else {
      value = (1 << node);
    }
  }
  bool valid() {
    return (value != 0);
  }
  bool isWrite() {
    return (value & (1 << 16));
  }
  uint32_t writer() {
    DBG_Assert(isWrite());
    return (value & 0xf);
  }
  uint32_t readers() {
    DBG_Assert(!isWrite());
    return (value & 0xffff);
  }
  bool isReader(uint32_t node) {
    DBG_Assert(!isWrite());
    return (value & (1 << node));
  }
  void addReader(uint32_t node) {
    DBG_Assert(!isWrite());
    value |= (1 << node);
  }
  void clear() {
    value = 0;
  }

  int32_t count() {
    int32_t ret = 0;
    uint32_t sharers = value;
    for (int32_t ii = 0; ii < 16; ii++) {
      if (sharers & 0x01) {
        ret++;
      }
      sharers >>= 1;
    }
    return ret;
  }

  uint32_t value;
};

class SignatureMapping32Bit {
public:
  // use at most 32-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word32Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping32Bit(uint32_t histDepth, uint32_t blockAddrBits, uint32_t blockSize)
    : myHistDepth(histDepth)
    , myAddrBits(blockAddrBits)
    , myBlockBits(log_base2(blockSize))
  {}

  Signature encodeHistory(HistoryEvent * histEvents) {
    uint32_t hist = 0;
    uint32_t offset = 0;

    // put the newest history event in the lowest bits
    for (uint32_t ii = 0; ii < myHistDepth; ++ii) {
      if (histEvents[ii].valid()) {
        if (histEvents[ii].isWrite()) {
          DBG_(VVerb, ( << "hist event " << ii << ": writer=" << histEvents[ii].writer() ) );
          hist |= (histEvents[ii].writer() << offset);
          hist |= (1 << (4 + offset));
          offset += 5;
        } else {
          DBG_(VVerb, ( << "hist event " << ii << ": readers=" << std::hex << histEvents[ii].readers() ) );
          hist |= (histEvents[ii].readers() << offset);
          offset += 17;
        }
      }
    }
    // make sure we haven't gone too far
    DBG_Assert(offset <= 32);
    DBG_(VVerb, ( << "created history trace 0x" << std::hex << hist ) );

    return Signature(hist);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHistory) {
    uint32_t sig = aHistory;

    // first shift and truncate the address
    uint32_t addr = (anAddr >> myBlockBits);
    addr &= (1 << myAddrBits) - 1;

    // then XOR the address with the history
    sig ^= addr;
    DBG_(VVerb, ( << "from addr 0x" << std::hex << anAddr << " created signature 0x" << sig ) );
    return Signature(sig);
  }

  const uint32_t myHistDepth;
  const uint32_t myAddrBits;
  const uint32_t myBlockBits;
};  // end class SignatureMapping32Bit

class SignatureMapping64Bit {
public:
  // use at most 64-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word64Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping64Bit(uint32_t histDepth, uint32_t blockAddrBits, uint32_t blockSize)
    : myHistDepth(histDepth)
    , myAddrBits(blockAddrBits)
    , myBlockBits(log_base2(blockSize))
  {}

  Signature encodeHistory(HistoryEvent * histEvents) {
    int64_t hist = 0;
    uint32_t offset = 0;

    // put the newest history event in the lowest bits
    for (uint32_t ii = 0; ii < myHistDepth; ++ii) {
      if (histEvents[ii].valid()) {
        if (histEvents[ii].isWrite()) {
          DBG_(VVerb, ( << "hist event " << ii << ": writer=" << histEvents[ii].writer() ) );
          hist |= ((int64_t)histEvents[ii].writer() << offset);
          hist |= ((int64_t)1 << (4 + offset));
          offset += 5;
        } else {
          DBG_(VVerb, ( << "hist event " << ii << ": readers=" << std::hex << histEvents[ii].readers() ) );
          hist |= ((int64_t)histEvents[ii].readers() << offset);
          offset += 17;
        }
      }
    }
    // make sure we haven't gone too far
    DBG_Assert(offset <= 64);
    DBG_(VVerb, ( << "created history trace 0x" << std::hex << hist ) );

    return Signature(hist);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHistory) {
    int64_t sig = aHistory;

    // first shift and truncate the address
    int64_t addr = (anAddr >> myBlockBits);
    addr &= ((int64_t)1 << myAddrBits) - (int64_t)1;

    // then XOR the address with the history
    sig ^= addr;
    DBG_(VVerb, ( << "from addr 0x" << std::hex << anAddr << " created signature 0x" << sig ) );
    return Signature(sig);
  }

  const uint32_t myHistDepth;
  const uint32_t myAddrBits;
  const uint32_t myBlockBits;
};  // end class SignatureMapping64Bit

template < int32_t N >
struct SignatureMappingN;

template<> struct SignatureMappingN<2> {
  typedef Flexus::Core::Address32Bit AddressLength;
  typedef SignatureMapping32Bit SignatureMapping;
};
template<> struct SignatureMappingN<3> {
  typedef Flexus::Core::Address32Bit AddressLength;
  typedef SignatureMapping32Bit SignatureMapping;
};
template<> struct SignatureMappingN<4> {
  typedef Flexus::Core::Address64Bit AddressLength;
  typedef SignatureMapping64Bit SignatureMapping;
};
template<> struct SignatureMappingN<5> {
  typedef Flexus::Core::Address64Bit AddressLength;
  typedef SignatureMapping64Bit SignatureMapping;
};

}  // end namespace nMrpTable
