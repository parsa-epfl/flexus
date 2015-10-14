#ifndef __REGION_DIRECTORY_HPP__
#define __REGION_DIRECTORY_HPP__

#include <algorithm>

#include <components/Common/Util.hpp>

#include <components/Common/Serializers.hpp>

using nCommonUtil::log_base2;
using nCommonSerializers::RegionDirEntrySerializer;

#include <components/CMPDirectory/AbstractRegionDirectory.hpp>

namespace nCMPDirectory {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

template < typename _State, typename _EState = _State >
class RegionDirectory : public AbstractRegionDirectory<_State, _EState> {

public:

  typedef MemoryAddress Tag;

  class RegionLookupResult;

  class Region {
  public:
    Region(int32_t blocksPerRegion, int32_t numSharers)
      : theTag(0)
      , theState(blocksPerRegion, _State(numSharers))
      , theProtected(blocksPerRegion, 0)
      , theOwner(-1)
    { }

    Tag tag() const {
      return (theTag);
    }
    const _State & state(int32_t offset) const {
      return theState[offset];
    }
    const std::vector<_State>& state() const {
      return theState;
    }
    int32_t owner() const {
      return theOwner;
    };

    void setTag(Tag aTag) {
      theTag = aTag;
    }
    void setState(int32_t offset, const _State & aState) {
      theState[offset] = aState;
    }
    void setOwner(int32_t anOwner) {
      DBG_(Trace, ( << "Setting owner of Region: " << std::hex << theTag << " to " << std::dec << anOwner ));
      theOwner = anOwner;
    }
    void setState(const std::vector<_State> &aState) {
      DBG_Assert( aState.size() == theState.size() );
      theState = aState;
    }
    void setSharerState(int32_t sharer, boost::dynamic_bitset<uint64_t> &presence, boost::dynamic_bitset<uint64_t> &exclusive) {
      setPresence(sharer, presence, exclusive, theState);
    }

    void setProtected(int32_t offset, bool val) {
      theProtected[offset] = val;
    }

    void clearState(const _State & aState) {
      theProtected.reset();
      typename std::vector<_State>::iterator iter = theState.begin();
      for (; iter != theState.end(); iter++) {
        (*iter) = aState;
        iter->reset();
      }
      theOwner = -1;
    }

    bool isProtected() {
      return theProtected.any();
    }

    void fillSerializer(RegionDirEntrySerializer & serializer) {
      serializer.tag = (uint64_t)theTag;
      serializer.num_blocks = (uint8_t)theState.size();
      serializer.owner = theOwner;
      serializer.state.clear();
      for (uint32_t i = 0; i < theState.size(); i++) {
        serializer.state.push_back(theState[i]);
      }
    }
    void operator=(const RegionDirEntrySerializer & serializer) {
      theTag = MemoryAddress(serializer.tag);
      DBG_Assert(theState.size() == serializer.num_blocks);
      theOwner = serializer.owner;
      for (uint32_t i = 0; i < serializer.num_blocks; i++) {
        theState[i] = serializer.state[i];
      }
      if (theOwner != -1) {
        DBG_(Verb, ( << "Region: 0x" << std::hex << (uint64_t)theTag << " owned by " << std::dec << (int)theOwner ));
      }
    }

  private:
    Tag theTag;
    std::vector<_State> theState;
    boost::dynamic_bitset<uint64_t> theProtected;
    int32_t theOwner;

    friend class RegionDirectory;
    friend class RegionLookupResult;
  };

  typedef std::vector<Region> RegionSet;

  class RegionLookupResult : public AbstractRegionLookupResult<_State> {
  public:
    virtual ~RegionLookupResult() {}
    RegionLookupResult(RegionSet * aSet,
                       Region  * aRegion,
                       int    anOffset,
                       bool   anIsValid)
      : theSet(aSet)
      , theRegion(aRegion)
      , theOffset(anOffset)
      , isValid(anIsValid)
    { }

    virtual bool found() {
      return isValid;
    }
    virtual void setProtected(bool val) {
      theRegion->setProtected(theOffset, val);
    }
    virtual const _State & state() const {
      return theRegion->state(theOffset);
    }

    virtual void addSharer(int32_t sharer) {
      theRegion->theState[theOffset].addSharer(sharer);
    }
    virtual void removeSharer(int32_t sharer) {
      theRegion->theState[theOffset].removeSharer(sharer);
    }
    virtual void setSharer(int32_t sharer) {
      theRegion->theState[theOffset].setSharer(sharer);
    }
    virtual void setState(const _State & state) {
      theRegion->theState[theOffset] = state;
    }

    virtual int32_t owner() const {
      return theRegion->owner();
    }

    virtual const _State & regionState(int32_t i) const {
      return theRegion->state(i);
    }
    virtual const std::vector<_State>& regionState() const {
      return theRegion->state();
    }

    virtual MemoryAddress regionTag() const {
      return theRegion->tag();
    }

    virtual void setRegionOwner(int32_t new_owner) {
      theRegion->setOwner(new_owner);
    }
    virtual void setRegionState(const std::vector<_State> &new_state) {
      theRegion->setState(new_state);
    }
    virtual void setRegionSharerState(int32_t sharer, boost::dynamic_bitset<uint64_t> &presence, boost::dynamic_bitset<uint64_t> &exclusive) {
      theRegion->setSharerState(sharer, presence, exclusive);
    }

  private:
    RegionSet * theSet;
    Region  * theRegion;
    int    theOffset;
    bool   isValid;

    friend class RegionDirectory<_State, _EState>;
  };

  class RegionEvictBuffer : public AbstractRegionEvictBuffer<_EState> {
  private:
    int32_t theBlocksPerRegion;
    int32_t theBlockSize;

    MemoryAddress theTagMask;

    std::string theName;

    MemoryAddress makeTag(MemoryAddress addr) {
      return MemoryAddress(addr & theTagMask);
    }

  public:
    struct BlockEvictEntry;
  private:

    struct REBEntry {
      REBEntry(MemoryAddress addr) : theAddress(addr), theInvalidatesPending(0) {}
      MemoryAddress       theAddress;
      mutable std::list<BlockEvictEntry> theBlockList;
      mutable int       theInvalidatesPending;
      mutable int       theOwner;
      mutable bool      theRevokeSent;
    };

    typedef multi_index_container
    < REBEntry
    , indexed_by
    < ordered_unique
    < member< REBEntry, MemoryAddress, &REBEntry::theAddress > >
    , sequenced <>
    >
    > evict_buf_t;

  public:

    typedef typename evict_buf_t::template nth_index<0>::type::iterator region_iterator;
    typedef typename evict_buf_t::template nth_index<1>::type::iterator order_iterator;

    class BlockEvictEntry : public AbstractEBEntry<_EState> {
      region_iterator theRegion;
      MemoryAddress theAddress;
      mutable _EState theState;
      mutable bool theInvalidatesSent;

    public:
      BlockEvictEntry(region_iterator r, MemoryAddress & a, const _EState & s)
        : theRegion(r), theAddress(a), theState(s), theInvalidatesSent(false)
      {}
      bool invalidatesRequired() const {
        return !theInvalidatesSent;
      }
      bool invalidatesPending() const {
        return theInvalidatesSent;
      }
      void setInvalidatesPending() const {
        theInvalidatesSent = true;
        theRegion->theInvalidatesPending++;
      }

      bool revokeRequired() const {
        return (theRegion->theOwner != -1) && !theRegion->theRevokeSent;
      }
      bool revokePending() const {
        return theRegion->theRevokeSent;
      }
      void setRevokePending() const {
        theRegion->theRevokeSent = true;
      }

      _EState & state() const {
        return theState;
      }
      const MemoryAddress & address() const {
        return theAddress;
      }
      bool operator<(const BlockEvictEntry & b) const {
        return theAddress < b.theAddress;
      }
      int & owner() const {
        return theRegion->theOwner;
      }

    };

    typedef typename std::list<BlockEvictEntry>::iterator block_iterator;

    evict_buf_t theEvictBuffer;

    friend class RegionDirectory<_State, _EState>;

  public:
    RegionEvictBuffer(const std::string & aName, int32_t size, int32_t blocksPerRegion, int32_t blockSize)
      : AbstractRegionEvictBuffer<_EState>(size) , theBlocksPerRegion(blocksPerRegion) , theBlockSize(blockSize), theName(aName) {
      uint64_t mask = (theBlocksPerRegion * theBlockSize) - 1;
      theTagMask = MemoryAddress(~mask);
    }

    region_iterator end() {
      return theEvictBuffer.end();
    }

    region_iterator findRegion(MemoryAddress anAddress) {
      MemoryAddress tag(makeTag(anAddress));
      return theEvictBuffer.find(tag);
    }

    virtual const AbstractEBEntry<_EState>* find(MemoryAddress anAddress) {
      MemoryAddress tag(makeTag(anAddress));
      region_iterator r_iter = theEvictBuffer.find(tag);
      if (r_iter == theEvictBuffer.end()) {
        return nullptr;
      }
      block_iterator b_iter = r_iter->theBlockList.begin();
      for (; b_iter != r_iter->theBlockList.end() && b_iter->address() <= anAddress; b_iter++) {
        if (b_iter->address() == anAddress) {
          return &(*b_iter);
        }
      }
      return nullptr;
    }
    virtual const AbstractEBEntry<_EState>& back() {
      return theEvictBuffer.get<1>().back().theBlockList.back();
    }
    virtual const AbstractEBEntry<_EState>* oldestRequiringRevoke() {
      order_iterator o_iter = theEvictBuffer.get<1>().begin();
      for (; o_iter != theEvictBuffer.get<1>().end(); o_iter++) {
        if (o_iter->theOwner != -1 && !o_iter->theRevokeSent) {
          block_iterator b_iter = o_iter->theBlockList.begin();
          return &(*b_iter);
        }
      }
      return nullptr;
    }
    virtual const AbstractEBEntry<_EState>* oldestRequiringInvalidates() {
      order_iterator o_iter = theEvictBuffer.get<1>().begin();
      for (; o_iter != theEvictBuffer.get<1>().end(); o_iter++) {
        if ((o_iter->theOwner == -1) && (o_iter->theInvalidatesPending < (int)o_iter->theBlockList.size())) {
          block_iterator b_iter = o_iter->theBlockList.begin();
          for (; b_iter != o_iter->theBlockList.end(); b_iter++) {
            if (b_iter->invalidatesRequired()) {
              return &(*b_iter);
            }
            DBG_(Trace, ( << "Looking for Required Invalidate in Region: " << std::hex << o_iter->theAddress << ", block: " << b_iter->address() << ", invalidatesRequired: " << std::boolalpha << b_iter->invalidatesRequired() ));
          }
        } else {
          DBG_(Trace, ( << "No Idle Work for Region: " << std::hex << o_iter->theAddress << ", Inval Pending: " << std::dec << o_iter->theInvalidatesPending << ", owner: " << o_iter->theOwner << ", revoke sent: " << std::boolalpha << o_iter->theRevokeSent << ", blocks present: " << o_iter->theBlockList.size() ));
        }
      }
      return nullptr;
    }
    virtual bool idleWorkReady() const {
      order_iterator o_iter = theEvictBuffer.get<1>().begin();
      for (; o_iter != theEvictBuffer.get<1>().end(); o_iter++) {
        if ((o_iter->theOwner != -1 && !o_iter->theRevokeSent) ||
            ((o_iter->theOwner == -1) && (o_iter->theInvalidatesPending < (int)o_iter->theBlockList.size()))) {
          DBG_(Trace, ( << "Found Idle Work for Region: " << std::hex << o_iter->theAddress << ", Inval Pending: " << std::dec << o_iter->theInvalidatesPending << ", owner: " << o_iter->theOwner << ", revoke sent: " << std::boolalpha << o_iter->theRevokeSent << ", blocks present: " << o_iter->theBlockList.size() ));
          return true;
        }
      }
      return false;
    }
    virtual void insert(MemoryAddress addr, _EState state) {
      DBG_Assert(false);
    }
    void insert(Region & region) {
      int32_t i = 0;
      for (; i < theBlocksPerRegion; i++) {
        if (!region.state(i).noSharers()) {
          break;
        }
      }
      if (i == theBlocksPerRegion && region.owner() == -1) {
        return;
      }
      bool success;
      region_iterator r_iter;
      std::tie(r_iter, success) = theEvictBuffer.insert(REBEntry(MemoryAddress(region.tag())));
      //DBG_Assert(success, ( << theName << " Found " << std::hex << region.tag() << " in evict buffer" ));
      if (success) {
        DBG_(Trace, ( << theName << " Inserting " << std::hex << region.tag() << " into evict buffer" ));
      } else {
        DBG_(Trace, ( << theName << " Re-inserting " << std::hex << region.tag() << " in evict buffer" ));
      }
      MemoryAddress addr(region.tag());
      addr += i * theBlockSize;
      for (; i < theBlocksPerRegion; i++, addr += theBlockSize) {
        if (!region.state(i).noSharers()) {
          DBG_(Trace, ( << theName << " Inserting " << std::hex << addr << " into evict buffer" ));
          r_iter->theBlockList.push_back(BlockEvictEntry(r_iter, addr, region.state(i)));
        }
      }
      if (!success) {
        r_iter->theBlockList.sort();
      }
      r_iter->theOwner = region.owner();
      if (region.owner() != -1) {
        DBG_(Trace, ( << "Inserting Region: " << std::hex << region.tag() << " into EB while owned by node: " << std::dec << r_iter->theOwner ));
      }
      EvictBuffer<_EState>::theCurSize++;
    }
    void updateState(region_iterator r_iter, std::vector<_State> &state) {
      r_iter->theBlockList.clear();
      MemoryAddress addr = r_iter->theAddress;
      for (int32_t i = 0; i < theBlocksPerRegion; i++, addr += theBlockSize) {
        if (!state[i].noSharers()) {
          DBG_(Trace, ( << theName << " Inserting " << std::hex << addr << " into evict buffer" ));
          r_iter->theBlockList.push_back(BlockEvictEntry(r_iter, addr, state[i]));
        }
      }
    }
    void updateSharerState(region_iterator r_iter, int32_t sharer, boost::dynamic_bitset<uint64_t> presence, boost::dynamic_bitset<uint64_t> &exclusive, _State & defaultState) {
      block_iterator b_iter = r_iter->theBlockList.begin();
      block_iterator next_block = b_iter;
      for (; b_iter != r_iter->theBlockList.end(); b_iter = next_block) {
        next_block++;
        int32_t index = ((uint64_t)b_iter->address() - (uint64_t)r_iter->theAddress) / theBlockSize;
        if (b_iter->state().isSharer(sharer) && !presence[index]) {
          b_iter->state().removeSharer(sharer);
          if (b_iter->state().noSharers()) {
            r_iter->theBlockList.erase(b_iter);
          }
        } else if (presence[index]) {
          if (exclusive[index]) {
            b_iter->state().setSharer(sharer);
          } else if (!b_iter->state().isSharer(sharer)) {
            b_iter->state().addSharer(sharer);
          }
          presence[index] = false;
        }
      }
      MemoryAddress addr = r_iter->theAddress;
      for (int32_t i = 0; i < theBlocksPerRegion; i++, addr += theBlockSize) {
        if (presence[i]) {
          r_iter->theBlockList.push_back(BlockEvictEntry(r_iter, addr, defaultState));
          r_iter->theBlockList.back().state().setSharer(sharer);
        }
      }
      r_iter->theBlockList.sort();
    }
    void remove(MemoryAddress anAddress) {
      MemoryAddress tag(makeTag(anAddress));
      region_iterator r_iter = theEvictBuffer.find(tag);
      if (r_iter == theEvictBuffer.end()) {
        return;
      }
      block_iterator b_iter = r_iter->theBlockList.begin();
      block_iterator next = b_iter;
      next++;
      for (; b_iter != r_iter->theBlockList.end() && b_iter->address() <= anAddress; b_iter = next, next++) {
        if (b_iter->address() == anAddress) {
          if (b_iter->invalidatesPending()) {
            r_iter->theInvalidatesPending--;
          }
          r_iter->theBlockList.erase(b_iter);
          if (r_iter->theBlockList.size() == 0) {
            DBG_(Trace, ( << theName << " Removing block Addr:0x" << std::hex << anAddress << " and region " << r_iter->theAddress << " from evict buffer" ));
            theEvictBuffer.erase(r_iter);
            EvictBuffer<_EState>::theCurSize--;
            break;
          }
          DBG_(Trace, ( << theName << " Removing block Addr:0x" << std::hex << anAddress << " from evict buffer, but leaving region" ));
          break;
        }
      }
    }

    virtual bool empty() {
      return (theEvictBuffer.size() == 0);
    }
    virtual bool full() {
      return ((EvictBuffer<_EState>::theCurSize + EvictBuffer<_EState>::theReserve) >= EvictBuffer<_EState>::theSize);
    }
    virtual bool freeSlotsPending() {
      order_iterator o_iter = theEvictBuffer.get<1>().begin();
      if (o_iter != theEvictBuffer.get<1>().end()) {
        bool slot_pending = (o_iter->theInvalidatesPending == (int)o_iter->theBlockList.size());
        return (slot_pending || ((EvictBuffer<_EState>::theCurSize + EvictBuffer<_EState>::theReserve) < EvictBuffer<_EState>::theSize));
      }
      return ((EvictBuffer<_EState>::theCurSize + EvictBuffer<_EState>::theReserve) < EvictBuffer<_EState>::theSize);
    }

  };

private:
  int32_t theBlocksPerRegion;
  int32_t theNumSharers;
  int32_t theAssociativity;

  int32_t theBlockSize;
  int32_t theBanks;
  int32_t theBankIndex;
  int32_t theBankInterleaving;
  int32_t theRegionSize;
  int32_t theEBSize;

  int32_t theNumSets;

  std::string theName;

  Tag tagMask;

  int32_t setHighShift;
  int32_t setLowShift;
  uint64_t setLowMask;
  uint64_t setHighMask;
  bool theSkewSet;
  int32_t setSkewShift;

  int32_t theBlockShift;
  int32_t theOffsetMask;

  int32_t theBankShift;
  int32_t theBankMask;

  std::vector<RegionSet> theSets;

  RegionEvictBuffer * theEvictBuffer;

  inline MemoryAddress makeTag(MemoryAddress anAddress) {
    return MemoryAddress(anAddress & tagMask);
  }

  inline int32_t makeSet(MemoryAddress addr) {
    if (theSkewSet) {
      addr = MemoryAddress(addr ^ (addr >> setSkewShift));
    }
    return ((addr >> setLowShift) & setLowMask) | ((addr >> setHighShift) & setHighMask);
  }

  inline int32_t makeOffset(MemoryAddress addr) {
    return ((addr >> theBlockShift) & theOffsetMask);
  }

  inline int32_t makeBank(MemoryAddress addr) {
    return (addr >> theBankShift) & theBankMask;
  }

public:
  typedef AbstractRegionLookupResult<_State> ARLookupResult;
  typedef typename boost::intrusive_ptr<ARLookupResult> ARLookupResult_p;
  typedef RegionLookupResult LookupResult;
  typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

  virtual ~RegionDirectory() { }

  RegionDirectory(const DirectoryInfo & theInfo, const std::list< std::pair< std::string, std::string> > &theConfiguration) {
    theBlockSize = theInfo.theBlockSize;
    theBanks = theInfo.theNumBanks;
    theBankInterleaving = theInfo.theBankInterleaving;
    theBankIndex = theInfo.theNodeId;
    theNumSharers = theInfo.theCores;
    theEBSize = theInfo.theEBSize;
    theName = theInfo.theName;
    theSkewSet = false;

    std::list< std::pair< std::string, std::string> >::const_iterator iter = theConfiguration.begin();
    for (; iter != theConfiguration.end(); iter++) {
      if (iter->first == "sets") {
        theNumSets = strtoll(iter->second.c_str(), nullptr, 0);
      } else if (iter->first == "total_sets" || iter->first == "global_sets") {
        int32_t global_sets = strtol(iter->second.c_str(), nullptr, 0);
        theNumSets = global_sets / theBanks;
        DBG_Assert( (theNumSets * theBanks) == global_sets, ( << "global_sets (" << global_sets << ") is not divisible by number of banks (" << theBanks << ")" ));
      } else if (iter->first == "rsize" || iter->first == "region_size") {
        theRegionSize = strtol(iter->second.c_str(), nullptr, 0);
      } else if (iter->first == "assoc" || iter->first == "associativity") {
        theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if (iter->first == "skew" || iter->first == "skew_set") {
        theSkewSet = boost::lexical_cast<bool>(iter->second);
      } else {
        DBG_Assert( false, ( << "Unknown configuration parameter '" << iter->first << "' while creating RegionDirectory" ));
      }
    }

    init();
  }

  void init() {
    DBG_Assert ( theNumSets > 0 );
    DBG_Assert ( theAssociativity > 0 );
    DBG_Assert ( theBlockSize     > 0 );
    DBG_Assert ( theRegionSize     > 0 );

    theBlocksPerRegion = theRegionSize / theBlockSize;
    DBG_Assert( (theBlocksPerRegion * theBlockSize) == theRegionSize );

    //  Physical address layout:
    //
    //  +---------+------------+------+-----------+----------------------------+
    //  | Tag     | Index High | Bank | Index Low | RegionOffset | BlockOffset |
    //  +---------+------------+------+-----------+----------------------- ----+
    //                                            |<------ setLowShift ------->|
    //                                |<--------->|
    //           setMaskLow

    DBG_Assert( ((theNumSets - 1) & theNumSets) == 0);
    DBG_Assert( ((theBanks - 1) & theBanks) == 0);
    DBG_Assert( ((theBlockSize - 1) & theBlockSize) == 0);
    DBG_Assert( ((theRegionSize - 1) & theRegionSize) == 0);
    DBG_Assert( ((theBankInterleaving - 1) & theBankInterleaving) == 0);
    DBG_Assert( theBankInterleaving >= theRegionSize );

    int32_t blockOffsetBits = log_base2(theBlockSize);
    int32_t regionOffsetBits = log_base2(theRegionSize);
    int32_t bankBits = log_base2(theBanks);
    int32_t indexBits = log_base2(theNumSets);
    int32_t interleavingBits = log_base2(theBankInterleaving);

    int32_t lowBits = interleavingBits - regionOffsetBits;
    int32_t highBits = indexBits - lowBits;

    setLowMask = (1 << lowBits) - 1;
    setHighMask = ((1 << highBits) - 1) << lowBits;

    setLowShift = regionOffsetBits;
    setHighShift = bankBits + regionOffsetBits;

    setSkewShift = 34 - (indexBits + bankBits + regionOffsetBits);

    tagMask = MemoryAddress(~0ULL & ~((uint64_t)(theRegionSize - 1)));

    theBlockShift = blockOffsetBits;
    theOffsetMask = theBlocksPerRegion - 1;

    theBankShift = interleavingBits;
    theBankMask = theBanks - 1;

    DBG_(Tmp, ( << "Create Region Directory with " << theNumSets << " sets and " << theAssociativity << " ways. BPR = " << theBlocksPerRegion << ", NumSharers = " << theNumSharers ));
    theSets.resize(theNumSets, RegionSet(theAssociativity, Region(theBlocksPerRegion, theNumSharers)));

    theEvictBuffer = new RegionEvictBuffer(theName, theEBSize, theBlocksPerRegion, theBlockSize);
  }

  MemoryAddress getRegion(MemoryAddress addr) {
    return makeTag(addr);
  }
  int32_t getBank(MemoryAddress addr) {
    return makeBank(addr);
  }

  ~RegionDirectory() {
    delete theEvictBuffer;
  }

  int32_t blocksPerRegion() {
    return theBlocksPerRegion;
  }

  std::pair<int, bool> selectVictim(RegionSet & set) {
    int32_t min_blocks = theBlocksPerRegion + 1;
    int32_t min_sharers = theNumSharers + 1;
    int32_t min_way = -1;
    for (int32_t i = 0; i < theAssociativity; i++) {
      if (!set[i].isProtected()) {
        int32_t bcount = 0;
        int32_t max_sharers = 0;
        for (int32_t j = 0; j < theBlocksPerRegion; j++) {
          int32_t s = set[i].state(j).countSharers();
          if (s > 0) {
            bcount++;
            if (s > max_sharers) {
              max_sharers = s;
            }
          }
        }
        if (bcount == 0) {
          return std::make_pair(i, false);
        }
        if (bcount < min_blocks) {
          min_blocks = bcount;
          min_sharers = max_sharers;
          min_way = i;
        } else if (bcount == min_blocks && max_sharers < min_sharers) {
          min_blocks = bcount;
          min_sharers = max_sharers;
          min_way = i;
        }
      }
    }
    return std::make_pair(min_way, true);
  }

  virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress address, const _State & state) {
    RegionLookupResult * reg_lookup = dynamic_cast<RegionLookupResult *>(lookup.get());
    DBG_Assert(reg_lookup != nullptr, ( << "allocate() was not passed a valid RegionLookupResult"));

    RegionSet & set = *(reg_lookup->theSet);

    int32_t way;
    bool was_valid;
    std::tie(way, was_valid) = selectVictim(set);
    if (way < 0) {
      // If there are no "un-protected" ways we fail
      return false;
    }

    Region victim = set[way];

    set[way].setTag(makeTag(address));
    set[way].clearState(state);
    set[way].setState(makeOffset(address), state);

    reg_lookup->theRegion = &(set[way]);
    reg_lookup->theOffset = makeOffset(address);
    reg_lookup->isValid   = true;

    if (was_valid) {
      theEvictBuffer->insert(victim);
    }

    // Check if this region is in the Evict buffer
    typename RegionEvictBuffer::region_iterator r_iter = theEvictBuffer->findRegion(address);
    if (r_iter != theEvictBuffer->end()) {
      typename RegionEvictBuffer::block_iterator b_iter = r_iter->theBlockList.begin();
      typename RegionEvictBuffer::block_iterator next = b_iter;
      for (next++; b_iter != r_iter->theBlockList.end(); b_iter = next, next++) {
        if (!b_iter->invalidatesPending()) {
          DBG_(Trace, ( << theName << " Moving block Addr:0x" << std::hex << b_iter->address() << " from Evict Buffer back to RegionDirectory" ));
          set[way].theState[makeOffset(b_iter->address())] =  b_iter->state();
          r_iter->theBlockList.erase(b_iter);
        }
      }
      if (r_iter->theBlockList.empty()) {
        theEvictBuffer->theEvictBuffer.erase(r_iter);
      }
    }

    return true;
  }

  // Bypasses the Evict Buffer
  virtual bool allocateRegion(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress & address, std::vector<_State> &state) {
    RegionLookupResult * reg_lookup = dynamic_cast<RegionLookupResult *>(lookup.get());
    DBG_Assert(reg_lookup != nullptr, ( << "allocate() was not passed a valid RegionLookupResult"));

    RegionSet & set = *(reg_lookup->theSet);

    int32_t way;
    bool was_valid;
    std::tie(way, was_valid) = selectVictim(set);
    if (way < 0) {
      // If there are no "un-protected" ways we fail
      return false;
    }

    Region victim = set[way];

    set[way].setTag(makeTag(address));
    set[way].setState(state);

    reg_lookup->theRegion = &(set[way]);
    reg_lookup->theOffset = makeOffset(address);
    reg_lookup->isValid   = true;

    std::for_each(state.begin(), state.end(), std::bind(&_State::reset, _1));
    if (was_valid) {
      state = victim.state();
      address = victim.tag();
    } else {
      // We can use 1 as a invalid address because any other address should be region-aligned
      address = MemoryAddress(1);
    }

    return true;
  }

  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > lookup(MemoryAddress address) {
    return nativeLookup(address);
  }

  // Provide access to the RegionLookupResult directly
  virtual ARLookupResult_p nativeLookup(MemoryAddress address) {
    RegionSet & set = theSets[makeSet(address)];

    Tag tag = makeTag(address);
    for (int32_t i = 0; i < theAssociativity; i++) {
      if (set[i].tag() == tag) {
        return LookupResult_p(new LookupResult(&set, &(set[i]), makeOffset(address), true));
      }
    }
    return LookupResult_p(new LookupResult(&set, nullptr, makeOffset(address), false));
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) {
    return (makeSet(a) == makeSet(b));
  }

  virtual EvictBuffer<_EState>* getEvictBuffer() {
    return theEvictBuffer;
  }

  virtual AbstractRegionEvictBuffer<_EState>* getRegionEvictBuffer() {
    return theEvictBuffer;
  }

  virtual bool loadState(std::istream & is) {
    try {
      boost::archive::binary_iarchive ia(is);
      int32_t setsPerGroup = theBankInterleaving / theRegionSize;
      int32_t numGroups = theNumSets / setsPerGroup;
      int32_t set = 0;
      RegionDirEntrySerializer serializer(0, theBlocksPerRegion, -1);
      int32_t global_set = 0;
      for (int32_t group = 0; group < numGroups; group++) {
        for (int32_t bank = 0; bank < theBanks; bank++) {
          if (bank == theBankIndex) {
            for (int32_t local_set = 0; local_set < setsPerGroup; local_set++, set++, global_set++) {
              DBG_Assert(set < theNumSets);
              for (int32_t way = 0; way < theAssociativity; way++) {
                ia >> serializer;
                DBG_(Trace, ( << "Dir " << theBankIndex << " Loading DirEntry: ( " << global_set << "/" << set << ", " << way << ") = " << serializer ));
                theSets[set][way] = serializer;
              }
            }
          } else {
            for (int32_t local_set = 0; local_set < setsPerGroup; local_set++, global_set++) {
              for (int32_t way = 0; way < theAssociativity; way++) {
                // Skip over entries that belong to other banks
                ia >> serializer;
              }
            }
          }
        }
      }
      DBG_(Tmp, ( << "Loaded " << set << " sets and " << theAssociativity << " ways." ));
    } catch (std::exception) {
      return false;
    }
    return true;
  }

};

}; // namespace nCMPDirectory

#endif // __REGION_DIRECTORY_HPP__

