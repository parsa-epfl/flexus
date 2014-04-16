namespace nDgpTable {

uint32_t log_base2(uint32_t num) {
  uint32_t ii = 0;
  while (num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

uint32_t max(uint32_t a, uint32_t b) {
  return (a > b ? a : b);
}

class SignatureMapping {
public:
  // assume at most 32-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word32Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping(uint32_t blockAddrBits, uint32_t pcBits, uint32_t blockSize)
    : myHistBits(pcBits)
    , myAddrBits(blockAddrBits)
    , mySigBits(max(pcBits, blockAddrBits))
    , myBlockBits(log_base2(blockSize))
  {}

  Signature makeHist(MemoryAddress aPC) {
    // ignore the lowest two bits since PCs increment by 4
    uint32_t temp = aPC >> 2;
    // truncate
    temp &= (1 << myHistBits) - 1;
    //DBG_(Tmp, ( << "from pc=" << std::hex << aPC << ", made hist=" << temp ) );
    return Signature(temp);
  }

  Signature updateHist(Signature aHist, MemoryAddress aPC) {
    // ignore the lowest two bits since PCs increment by 4
    uint32_t temp = aPC >> 2;
    // use addition to munge the two together
    temp += aHist;
    // now truncate
    temp &= (1 << myHistBits) - 1;
    //DBG_(Tmp, ( << "from pc=" << std::hex << aPC << " hist=" << aHist <<", updated hist=" << temp ) );
    return Signature(temp);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHist) {
    // ignore bits that map within the coherency unit
    uint32_t temp = anAddr >> myBlockBits;
    // truncate the address
    temp &= (1 << myAddrBits) - 1;

    // shuffle left?
    temp <<= 8;

    // XOR with the history (should already be truncated)
    temp ^= aHist;
    // truncate (no longer necessary): temp &= (1<<mySigBits) - 1;
    //DBG_(Tmp, ( << "from addr=" << std::hex << anAddr << " hist=" << aHist << ", made sig=" << temp ) );
    return Signature(temp);
  }

  const uint32_t myHistBits;
  const uint32_t myAddrBits;
  const uint32_t mySigBits;
  const uint32_t myBlockBits;
};  // end class SignatureMapping

class Confidence {
  // the confidence is encoded in the lowest seven bits
  // the highest bit is set for the initial access
  uint8_t bitVector;

  // tracks if the confidence has flip-flopped
  //uint8_t flipFlop;

public:
#ifdef DGP_SUBTRACE
  // track subtraces
  std::set<Confidence *> tracePtrs;
  bool subtrace;
  std::vector<int> numPCseq;
#endif

public:
  Confidence()
    : bitVector(0x80 | theDGPInitialConf)
    //, flipFlop(0)
#ifdef DGP_SUBTRACE
    , subtrace(false)
#endif
  {}

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & bitVector;
  }

  void reset() {
    bitVector = 0x80 | theDGPInitialConf;   // init=true, conf=1
    //flipFlop = 0;
#ifdef DGP_SUBTRACE
    subtrace = false;
    tracePtrs.clear();
    numPCseq.clear();
#endif
  }

  void raise() {
    if (!initial()) {
      if (bitVector == (theDGPThresholdConf - 1)) {
        //flipFlop |= 0x1;
      }
    }

    bitVector &= 0x7f;  // clear init bit
    if (bitVector < theDGPMaxConf) {
      bitVector++;
    }
  }
  bool lower() {
    if (bitVector == theDGPThresholdConf) {
      //flipFlop |= 0x2;
    }

    bitVector &= 0x7f;  // clear init bit
    if (bitVector > 0) {
      bitVector--;
      return false;
    }
    return true;
  }

  void raise(DgpStats * stats, int32_t table) {
    if (!initial()) {
      if (bitVector == (theDGPThresholdConf - 1)) {
        //flipFlop |= 0x1;
      }
    }

    bitVector &= 0x7f;  // clear init bit
    if (bitVector < theDGPMaxConf) {
      int32_t val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector) + 1;
      stats->ConfTransitions << std::make_pair(val, 1);
      if (table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if (table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      bitVector++;
    } else {
      int32_t val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector);
      stats->ConfTransitions << std::make_pair(val, 1);
      if (table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if (table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
    }
  }
  bool lower(DgpStats * stats, int32_t table) {
    if (bitVector == theDGPThresholdConf) {
      //flipFlop |= 0x2;
    }

    bitVector &= 0x7f;  // clear init bit
    if (bitVector > 0) {
      int32_t val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector) - 1;
      stats->ConfTransitions << std::make_pair(val, 1);
      if (table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if (table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      bitVector--;
      return false;
    } else {
      int32_t val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector);
      stats->ConfTransitions << std::make_pair(val, 1);
      if (table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if (table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      return true;
    }
  }

  uint8_t val() {
    return (bitVector & 0x7f);
  }

  bool predict() {
    if ( (bitVector & 0x7f) >= theDGPThresholdConf ) {
      return true;
    }
    return false;
  }

  bool initial() {
    return (bitVector & 0x80);
  }

  bool min() {
    return (bitVector == 0);
  }

  bool max() {
    return ( (bitVector & 0x7f) == theDGPMaxConf );
  }

  bool isFlipping() {
    //return (flipFlop == 0x3);
    return false;
  }

#ifdef DGP_SUBTRACE
  bool isSubtrace() {
    return subtrace;
  }
  bool isSupertrace() {
    if (!subtrace) {
      if (!tracePtrs.empty()) {
        return true;
      }
    }
    return false;
  }

  int32_t numSubtraces() {
    if (subtrace) {
      return -1;
    }
    return tracePtrs.size();
  }
  Confidence * getSupertrace() {
    DBG_Assert(isSubtrace());
    DBG_Assert(tracePtrs.size() == 1);
    return *(tracePtrs.begin());
  }
  void addSubtraces(std::set<Confidence *> & newSubtraces) {
    subtrace = false;
    std::set<Confidence *>::iterator iter = newSubtraces.begin();
    while (iter != newSubtraces.end()) {
      (*iter)->subtrace = true;
      (*iter)->tracePtrs.clear();
      (*iter)->tracePtrs.insert(this);
      tracePtrs.insert(*iter);
      ++iter;
    }
  }
  void numStores(int32_t number) {
    numPCseq.push_back(number);
  }
#endif

  friend std::ostream & operator << (std::ostream & anOstream, const Confidence & aConf) {
    return anOstream << (int)(aConf.bitVector /*& 0x7f*/);
  }

};  // end class Confidence

}  // end namespace nDgpTable
