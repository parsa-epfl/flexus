namespace nDgpTable {

using namespace Flexus::Stat;

using boost::intrusive_ptr;

class DgpPredictionTable {
public:
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  struct PredEntry : boost::counted_base {
    PredEntry(Signature sig, bool pred, bool init, bool privilege, int32_t table, int64_t confs, DgpStats & theStats)
      : signature(sig)
      , predicted(pred)
      , initial(init)
      , priv(privilege)
      , sigTable(table)
      , confVals(confs)
      , PredCorrect(theStats.CorrectPredictions.predict())
      , PredMispredict(theStats.Mispredictions.predict())
      , PredDangling(theStats.DanglingPredictions.predict()) {
      if (predicted) {
        PredDangling->guess();
      }
    }

    Signature signature;
    bool predicted;
    bool initial;       // the initial prediction on this signature?
    bool priv;          // OS access?
    int32_t  sigTable;      // which signature table generated this prediction?
    int64_t confVals; // confidence values from both tables
    boost::intrusive_ptr<Prediction> PredCorrect;
    boost::intrusive_ptr<Prediction> PredMispredict;
    boost::intrusive_ptr<Prediction> PredDangling;

    enum ePredResult {
      kBad,
      kGood,
      kDangling
    };

    void resolve(ePredResult aResult) {
      switch (aResult) {
        case kBad:
          PredDangling->reject();
          PredMispredict->confirm();
          break;
        case kGood:
          PredDangling->reject();
          PredCorrect->confirm();
          break;
        case kDangling:
          PredDangling->goodGuess();
          break;
      }
      PredCorrect->dismiss();
      PredMispredict->dismiss();
      PredDangling->dismiss();
    }

  };

private:
  typedef intrusive_ptr<PredEntry> PredEntry_p;
  typedef std::map<MemoryAddress, PredEntry_p> PredMap;
  typedef PredMap::iterator Iterator;
  typedef std::pair<MemoryAddress, PredEntry_p> PredPair;
  typedef std::pair<Iterator, bool> InsertPair;

public:
  DgpPredictionTable(SignatureMapping & aMapper)
    : theSigMapper(aMapper)
  {}

  PredEntry_p findAndRemove(MemoryAddress anAddr) {
    Iterator iter = thePredictions.find(anAddr);
    if (iter != thePredictions.end()) {
      PredEntry_p temp = iter->second;
      thePredictions.erase(iter);
      return temp;
    }
    return PredEntry_p(0);
  }

  void addEntry(MemoryAddress anAddr, Signature aSig, bool aPrediction, bool initial, bool priv, int32_t aSigTable, int64_t aConfVals, DgpStats & theStats) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    PredEntry_p anEntry = new PredEntry(aSig, aPrediction, initial, priv, aSigTable, aConfVals, theStats);
    PredPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = thePredictions.insert(insert);
    DBG_Assert(result.second);
  }

  void finalizeStats() {
    DBG_(Iface, ( << "DGP Finalizing Stats") );
    //Process all predictions left in the prediction table
    Iterator iter = thePredictions.begin();
    Iterator end = thePredictions.end();
    while (iter != end) {
      iter->second->resolve(PredEntry::kDangling);
      ++iter;
    }
    thePredictions.clear();

    DBG_(Iface, ( << "DGP Stats Finalized") );
  }

private:
  // use a Map to allow fast lookup (also, we don't care about structure)
  PredMap thePredictions;
  SignatureMapping & theSigMapper;

};  // end class DgpPredictionTable

}  // end namespace nDgpTable
