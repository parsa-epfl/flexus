#include <components/CMPDirectory/SimpleDirectoryState.hpp>
#include <components/CMPDirectory/AbstractPolicy.hpp>
#include <components/CMPDirectory/AbstractDirectory.hpp>
#include <components/CMPDirectory/MissAddressFile.hpp>
#include <components/CMPDirectory/EvictBuffer.hpp>

namespace nCMPDirectory {

class InclusiveMOESIPolicy : public AbstractPolicy {

public:

  typedef SimpleDirectoryState State;

  InclusiveMOESIPolicy(const DirectoryInfo & params);

  virtual ~InclusiveMOESIPolicy();

  virtual void handleRequest( ProcessEntry_p process );
  virtual void handleSnoop( ProcessEntry_p process );
  virtual void handleReply( ProcessEntry_p process );
  virtual void handleWakeMAF( ProcessEntry_p process );
  virtual void handleEvict( ProcessEntry_p process );
  virtual void handleIdleWork( ProcessEntry_p process );

  virtual bool isQuiesced() const;

  virtual AbstractEvictBuffer & EB() {
    return *theEvictBuffer;
  }
  virtual MissAddressFile & MAF() {
    return theMAF;
  }

  virtual bool hasIdleWorkAvailable();
  virtual MemoryTransport getIdleWorkTransport();
  virtual MemoryTransport getEvictBlockTransport();

  virtual void wakeMAFs(MemoryAddress anAddress);

  // static memembers to work with AbstractFactory
  static AbstractPolicy * createInstance(std::list<std::pair<std::string, std::string> > &args, const DirectoryInfo & params);
  static const std::string name;

  virtual bool loadState(std::istream & is);

private:

  typedef boost::intrusive_ptr<MemoryMessage> MemoryMessage_p;

  typedef MissAddressFile::iterator maf_iter_t;

  typedef AbstractLookupResult<State> LookupResult;
  typedef boost::intrusive_ptr<LookupResult> LookupResult_p;

  void doRequest( ProcessEntry_p process, bool has_maf );
  bool allocateDirectoryEntry( LookupResult_p lookup, MemoryAddress anAddress, const State & aState);

  DirectoryInfo theDirectoryInfo;
  AbstractDirectory<SimpleDirectoryState> *theDirectory;
  EvictBuffer<SimpleDirectoryState>  *theEvictBuffer;
  MissAddressFile       theMAF;

  SimpleDirectoryState theDefaultState;

  int32_t pickSharer(const SimpleDirectoryState & state, int32_t requester, int32_t dir);

};

}; // namespace nCMPDirectory
