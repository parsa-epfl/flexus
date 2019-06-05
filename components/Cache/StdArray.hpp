#ifndef _CACHE_ARRAY_HPP
#define _CACHE_ARRAY_HPP

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <exception>
#include <iostream>
#include <stdlib.h>

#include <boost/throw_exception.hpp>
#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <components/CommonQEMU/Util.hpp>
#include <components/CommonQEMU/Serializers.hpp>

using nCommonUtil::log_base2;
using nCommonSerializers::BlockSerializer;

namespace nCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef int32_t SetIndex;
typedef uint32_t BlockOffset;

enum ReplacementPolicy {
  REPLACEMENT_LRU,
  REPLACEMENT_PLRU
};

template<typename _State, const _State & _DefaultState> class StreamBuffer;
// This is a cache block.  The accessor functions are braindead simple.
template<typename _State, const _State & _DefaultState>
class Block {
public:

  Block ( void ) : theTag ( 0 ), theState ( _DefaultState ), theStreamBuffer( NULL ) {}

  MemoryAddress tag ( void ) const {
    return theTag;
  }

  MemoryAddress & tag ( void ) {
    return theTag;
  }

  const _State & state( void ) const {
    return theState;
  }
  _State & state( void ) {
    return theState;
  }

  bool valid() {
    return theState != _DefaultState;
  }

  StreamBuffer<_State, _DefaultState>* associatedStreamBuffer() {
    return theStreamBuffer;
  }

  void associateStreamBuffer(StreamBuffer<_State, _DefaultState>   * aStreamBuffer) {
    theStreamBuffer = aStreamBuffer;
  }

  void freeStreamBuffer() {
    theStreamBuffer = NULL;
  }

private:
  MemoryAddress theTag;
  _State    theState;
  StreamBuffer<_State, _DefaultState>   * theStreamBuffer;

}; // class Block

template<typename _State, const _State & _DefaultState> class Set;
template<typename _State, const _State & _DefaultState> class StdArray;

// The output of a cache lookup
template<typename _State, const _State & _DefaultState>
class StdLookupResult : public AbstractLookupResult<_State> {
public:
  virtual ~StdLookupResult () {}
  StdLookupResult (  Set<_State, _DefaultState>   * aSet,
                     Block<_State, _DefaultState> * aBlock,
                     MemoryAddress                  aBlockAddress,
                     bool                           aIsHit ) :
    theSet          ( aSet ),
    theBlock        ( aBlock ),
    theBlockAddress ( aBlockAddress ),
    isHit           ( aIsHit ),
    theOrigState    ( aIsHit ? aBlock->state() : _DefaultState ) {
    DBG_Assert( (aBlock == nullptr) || (aBlock->tag() == aBlockAddress), ( << "Created Lookup where tag " << std::hex << (uint64_t)aBlock->tag() << " != address " << (uint64_t)aBlockAddress ));
  }

  const _State & state() const {
    return (isHit ? theBlock->state() : theOrigState );
  }
  void setState( const _State & aNewState) {
    theBlock->state() = aNewState;
  }
  void setProtected( bool val) {
    theBlock->state().setProtected(val);
  }
  void setPrefetched( bool val) {
    theBlock->state().setPrefetched(val);
  }

  bool hit  ( void ) const {
    return isHit;
  }
  bool miss ( void ) const {
    return !isHit;
  }
  bool found( void ) const {
    return theBlock != nullptr;
  }
  bool valid( void ) const {
    return theBlock != nullptr;
  }

  MemoryAddress blockAddress ( void ) const {
    return theBlockAddress;
  }

protected:
  Set<_State, _DefaultState>     * theSet;
  Block<_State, _DefaultState>  * theBlock;
  MemoryAddress     theBlockAddress;
  bool                isHit;
  _State       theOrigState;

  friend class Set<_State, _DefaultState>;
  friend class StdArray<_State, _DefaultState>;
}; // class LookupResult

/*
//the output of a StreamBuffer lookup
template<typename _State, const _State & _DefaultState>
class StreamBufferLookupResult : public AbstractLookupResult<_State> {
public:
  virtual ~StreamBufferLookupResult () {}
  StreamBufferLookupResult (  StreamBuffer<_State, _DefaultState>   * aStreamBuffer,
                     Block<_State, _DefaultState> * aBlock,
                     MemoryAddress                  aBlockAddress,
                     bool                           aIsHit ) :
    theStreamBuffer ( aStreamBuffer ),
    theBlock        ( aBlock ),
    theBlockAddress ( aBlockAddress ),
    isHit           ( aIsHit ),
    theOrigState    ( aIsHit ? aBlock->state() : _DefaultState ) {
    DBG_Assert( (aBlock == NULL) || (aBlock->tag() == aBlockAddress), ( << "Created Lookup where tag " << std::hex << (uint64_t)aBlock->tag() << " != address " << (uint64_t)aBlockAddress ));
  }

  const _State & state() const {
    return (isHit ? theBlock->state() : theOrigState );
  }
  void setState( const _State & aNewState) {
    theBlock->state() = aNewState;
  }
  void setProtected( bool val) {
    theBlock->state().setProtected(val);
  }
  void setPrefetched( bool val) {
    theBlock->state().setPrefetched(val);
  }

  bool hit  ( void ) const {
    return isHit;
  }
  bool miss ( void ) const {
    return !isHit;
  }
  bool found( void ) const {
    return theBlock != NULL;
  }
  bool valid( void ) const {
    return theBlock != NULL;
  }

  MemoryAddress blockAddress ( void ) const {
    return theBlockAddress;
  }

protected:
  StreamBuffer<_State, _DefaultState>     * theStreamBuffer;
  Block<_State, _DefaultState>  * theBlock;
  MemoryAddress     theBlockAddress;
  bool                isHit;
  _State       theOrigState;

  friend class StreamBuffer<_State, _DefaultState>;
  friend class StdArray<_State, _DefaultState>;
}; // class StreamBufferLookupResult
*/

template<typename _State, const _State & _DefaultState>
class StreamBufferEntry {
public:
  StreamBufferEntry() {
    theBlock = NULL;
    valid = false;
    theTag = (MemoryAddress)0;
  }
  virtual ~StreamBufferEntry() {}

  Block<_State, _DefaultState> * getBlock() {
    return theBlock;
  }

  void setBlock(Block<_State, _DefaultState> * aBlock) {
    theBlock = aBlock;
  }

  bool getValid() {
    return valid;
  }

  void setValid(bool aValid) {
    valid = aValid;
  }

  MemoryAddress getTag() {
    return theTag;
  }

  void setTag(MemoryAddress aTag) {
    theTag = aTag;
  }

protected:
  Block<_State, _DefaultState> * theBlock;
  bool valid;
  MemoryAddress theTag;
};

template<typename _State, const _State & _DefaultState>
class StreamBuffer {
public:
  StreamBuffer ( const uint32_t depth, const uint32_t anID ) {
    theDepth = depth;
    theSBEntries = new StreamBufferEntry<_State, _DefaultState>[theDepth];
    theIndex = 0;
    theATTidx = -1;
    bufferID = anID;
    clear();
  }
  virtual ~StreamBuffer() {}

  StreamBuffer<_State, _DefaultState> *lookupBlock ( const MemoryAddress anAddress ) {
    if (theSBEntries[0].getValid()
        && (uint64_t)anAddress >= (uint64_t)theSBEntries[0].getTag()
        && (uint64_t)anAddress <= (uint64_t)theSBEntries[0].getTag() + (theLength << 6)) { //WARNING: Hard-wired block size value (64B)
        return this;    //in short, if SABRe's lock already read and target address belongs in SABRe total range
    }
    int64_t index = ((int64_t)anAddress - (int64_t)theSBEntries[0].getTag()) >> 6;    //WARNING: Hard-wired block size value (64B)
    if (index < 0 || index >= (int)theDepth) {
      return NULL;  //not in this StrBuf
    }
    if (theSBEntries[index].getTag() == anAddress) {
        return this;
    }
    return NULL;
  }

/*  typedef StreamBufferLookupResult<_State, _DefaultState> StrBufLookupResult;
  typedef boost::intrusive_ptr<StrBufLookupResult>  StrBufLookupResult_p;

  StrBufLookupResult_p lookupBlock ( const MemoryAddress anAddress ) {
    int64_t index = ((int64_t)anAddress - (int64_t)theSBEntries[0].theTag) >> 6;    //WARNING: Hard-wired block size value (64B)
    if (index < 0 || index >= (int)theDepth) {
      return StrBufLookupResult_p( new StrBufLookupResult( this, NULL, anAddress, false ) );  //not in this StrBuf
    }

    int t = -1;
    if (theSBEntries[index].theTag == anAddress) {
        if ( theSBEntries[index].valid ) {
          return StrBufLookupResult_p( new StrBufLookupResult( this, theSBEntries[index].theBlock, anAddress, true ) );
        }
        t = index;
        if (t >= 0) {
          return StrBufLookupResult_p( new StrBufLookupResult( this, theSBEntries[t].theBlock, anAddress, false ) );
        }
    }
    return StrBufLookupResult_p( new StrBufLookupResult( this, NULL, anAddress, false ) );
  }

*/
  virtual int32_t slotsAvailable() {    //check if there is a free remaining entry in this Stream Buffer
    DBG_(RRPP_STREAM_BUFFERS, ( << "Checking availability for StreamBuffer with id " << bufferID << ", associated with ATTidx "
                                << theATTidx << ". theSBEntries[0].valid = " << theSBEntries[0].getValid() << ", theIndex = " << theIndex));
    if (theSBEntries[0].getValid()) return (theDepth);
    return (theDepth-theIndex);
  }

  virtual void reserveEntry() {   //reserve an entry in this stream buffer
    DBG_Assert(slotsAvailable());
    if (!theSBEntries[0].getValid()) {
      theSBEntries[theIndex].setTag((MemoryAddress)((uint64_t)theSBEntries[theIndex-1].getTag()+64));  //WARNING: Hard-wired block size value (64B)
      theIndex++;
    }
  }

  virtual void allocateEntry(Block<_State, _DefaultState> * aBlock) {   //allocate an entry in this stream buffer, when the data actually returs from memory and gets allocated in the array
    if (theSBEntries[0].getValid()) return;    //Lock's block has been acquired. No need to track anything anymore!
    int64_t index = ((int64_t)aBlock->tag() - (int64_t)theSBEntries[0].getTag()) >> 6;    //WARNING: Hard-wired block size value (64B)
    DBG_Assert(index >= 0 && index < theIndex);
    DBG_Assert(aBlock->tag() == theSBEntries[index].getTag());
    theSBEntries[index].setBlock(aBlock);
    theSBEntries[index].setValid(true);

    theSBEntries[index].getBlock()->state().setProtected(true);
    DBG_Assert(!theSBEntries[index].getBlock()->associatedStreamBuffer(), ( << "Was about to associate block with a Stream buffer, but block is already associated with one!"));
    theSBEntries[index].getBlock()->associateStreamBuffer(this);

    if (index == 0) { //lock acquired! No need to keep the rest of the blocks locked in the cache
      uint32_t i;
      for (i=1; i<theIndex; i++) {
        if (theSBEntries[i].getValid()) {
           theSBEntries[i].getBlock()->freeStreamBuffer();
           theSBEntries[i].getBlock()->state().setProtected(false);
           theSBEntries[i].setBlock(NULL);
        }
      }
    }
  }

  virtual void reserveStreamBuffer(int32_t anATTidx, uint64_t anAddress, uint32_t aLength) {  //should be called when RRPP stards a new atomic transfer, to match it to a Stream Buffer
    DBG_Assert(theATTidx == -1);
    theATTidx = anATTidx;
    theLength = aLength;
    theSBEntries[0].setTag((MemoryAddress)anAddress);
    theIndex++;
    DBG_(RRPP_STREAM_BUFFERS, ( << "\t...Reserving StreamBuffer with head address 0x" << std::hex << anAddress));
  }

  virtual int32_t invalidateStreamBuffer(MemoryAddress anAddress) {
    int64_t index = ((int64_t)anAddress - (int64_t)theSBEntries[0].getTag()) >> 6;    //WARNING: Hard-wired block size value (64B)
    DBG_Assert(index >= 0 && index < theIndex);
    DBG_Assert(theSBEntries[index].getTag() == anAddress && theSBEntries[index].getValid());
    int32_t theCanceledATT = theATTidx;
    clear();
    return theCanceledATT;
  }

  virtual void clear() {
   uint32_t i;
   for (i=0; i<theDepth; i++) {
    if (theSBEntries[i].getBlock()) {
      theSBEntries[i].getBlock()->freeStreamBuffer();
      theSBEntries[i].getBlock()->state().setProtected(false);
      theSBEntries[i].setBlock(NULL);
    }
    theSBEntries[i].setValid(false);
    theSBEntries[i].setTag((MemoryAddress)0);
   }
   theIndex = 0;
   theATTidx = -1;
   theLength = 0;
  }

  virtual int32_t getATTidx() {
    return theATTidx;
  }

  virtual uint32_t getBufferID() {
    return bufferID;
  }

  virtual uint32_t getIndex() {
    return theIndex;
  }

  StreamBufferEntry<_State, _DefaultState> *theSBEntries;
protected:
  uint32_t bufferID;
  uint32_t theLength;   //the length of the associated SABRe in blocks
  uint32_t theDepth, theIndex;
  int32_t theATTidx;   //buffer's corresponding ATT index in the RRPP
};

// The cache set, implements a set of cache blocks and applies a replacement policy
// (from a derived class)
template<typename _State, const _State & _DefaultState>
class Set {
public:
  Set ( const int32_t aAssociativity ) {
    theAssociativity = aAssociativity;
    theBlocks        = new Block<_State, _DefaultState>[theAssociativity];
    DBG_Assert ( theBlocks );
  }
  virtual ~Set ( ) {}

  typedef StdLookupResult<_State, _DefaultState> LookupResult;
  typedef boost::intrusive_ptr<LookupResult>  LookupResult_p;

  LookupResult_p lookupBlock ( const MemoryAddress anAddress ) {
    int32_t i, t = -1;

    // Linearly search through the set for the matching block
    // This could be made faster for higher-associativity sets
    // Through other search methods
    for ( i = 0; i < theAssociativity; i++ ) {
      if ( theBlocks[i].tag() == anAddress ) {
        if ( theBlocks[i].valid() ) {
          return LookupResult_p( new LookupResult( this, &(theBlocks[i]), anAddress, true ) );
        }
        t = i;
      }
    }
    if (t >= 0) {
      return LookupResult_p( new LookupResult( this, &(theBlocks[t]), anAddress, false ) );
    }

    // Miss on this set
    return LookupResult_p( new LookupResult( this, nullptr, anAddress, false ) );
  }

  virtual bool  canAllocate(LookupResult_p lookup, MemoryAddress anAddress) {
    // First look for an invalid tag match
    if (lookup->theBlock != nullptr) {
      return true;
    } else {
      return victimAvailable();
    }
  }
  virtual LookupResult_p  allocate(LookupResult_p lookup, MemoryAddress anAddress) {
    // First look for an invalid tag match
    if (lookup->theBlock != nullptr) {
      lookup->isHit = true;
      DBG_Assert( lookup->theBlock->tag() == anAddress, ( << "Lookup Tag " << std::hex << (uint64_t)lookup->theBlock->tag() << " != " << (uint64_t)anAddress ));
      // don't need to change the lookup, just fix order and return no victim
      recordAccess(lookup->theBlock);
      return LookupResult_p( new LookupResult( this, nullptr, anAddress, false ) );

    } else {
      Block<_State, _DefaultState> *victim = pickVictim();

      // Create lookup result now and remember the block state
      LookupResult_p v_lookup( new LookupResult( this, victim, blockAddress(victim), true) );
      v_lookup->isHit = false;

      victim->tag() = anAddress;

      lookup->isHit = true;
      lookup->theOrigState = _DefaultState;
      lookup->theBlock = victim;
      victim->state() = _DefaultState;

      recordAccess(victim);
      return v_lookup;
    }
  }

  virtual Block<_State, _DefaultState>* pickVictim() = 0;
  virtual bool victimAvailable() = 0;
  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) = 0;
  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) = 0;

  virtual bool saveState ( std::ostream & s ) = 0;
  virtual bool loadState ( boost::archive::binary_iarchive & ia, int32_t theIndex, int32_t theSet ) = 0;
  virtual bool loadTextState ( std::istream & is, int32_t theIndex, int32_t theSet, int32_t theTagShift, int32_t theSetShift ) = 0;

  MemoryAddress blockAddress ( const Block<_State, _DefaultState> * theBlock ) {
    return theBlock->tag();
  }

  int32_t count ( const MemoryAddress & aTag ) {
    int32_t i, res = 0;

    for (i = 0; i < theAssociativity; i++) {
      if (theBlocks[i].tag() == aTag) {
        res++;
      }
    }

    return res;
  }

  std::list<MemoryAddress> getTags() {
    std::list<MemoryAddress> tag_list;
    for (int32_t i = 0; i < theAssociativity; i++) {
      if (theBlocks[i].state().isValid()) {
        tag_list.push_back(theBlocks[i].tag());
      }
    }
    return tag_list;
  }

protected:

  Block<_State, _DefaultState> * theBlocks;
  int32_t     theAssociativity;

}; // class Set

template<typename _State, const _State & _DefaultState>
class SetLRU : public Set<_State, _DefaultState> {
public:
  SetLRU( const int32_t aAssociativity )
    : Set<_State, _DefaultState> ( aAssociativity ) {
    theMRUOrder = new int[aAssociativity];

    for ( int32_t i = 0; i < aAssociativity; i++ ) {
      theMRUOrder[i] = i;
    }
  }

  virtual Block<_State, _DefaultState>* pickVictim() {
    for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
      int32_t index = theMRUOrder[i];
      if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected()) {
        return &(Set<_State, _DefaultState>::theBlocks[index]);
      } else if (Set<_State, _DefaultState>::theBlocks[index].state() == _DefaultState) {
        DBG_(Dev, ( << "picking Protected victim in Invalid state: " << std::hex << (Set<_State, _DefaultState>::theBlocks[index].tag()) ));
        Set<_State, _DefaultState>::theBlocks[index].state().setProtected(false);
        return &(Set<_State, _DefaultState>::theBlocks[index]);
      }
    }
    DBG_Assert(false, ( << "All blocks in the set are protected and valid."));
    return nullptr;
  }
  virtual bool victimAvailable() {
    for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
      int32_t index = theMRUOrder[i];
      if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected()) {
        // Found unprotected block
        return true;
      } else if (Set<_State, _DefaultState>::theBlocks[index].state() == _DefaultState) {
        // Found Invalid block
        DBG_(Dev, ( << "Found Invalid block that can be evicted."));
        return true;
      }
    }
    return false;
  }

  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    moveToHead( theBlockNum);
  }

  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    moveToTail( theBlockNum);
    //ALEX
    StreamBuffer<_State, _DefaultState>* theStrBuf = aBlock->associatedStreamBuffer();
    if (theStrBuf) {
        int32_t theCanceledATT = theStrBuf->invalidateStreamBuffer(aBlock->tag());
        //TODO: Somehow pass this ATT to RRPP
    }
  }

  virtual bool saveState ( std::ostream & s ) {
    return true;
  }

  virtual bool loadState ( boost::archive::binary_iarchive & ia, int32_t theIndex, int32_t theSet ) {
    BlockSerializer bs;
    for (int32_t i = (Set<_State, _DefaultState>::theAssociativity - 1); i >= 0; i--) {
      ia >> bs;
      DBG_(Trace, Addr(bs.tag) ( << theIndex << "-L2: Loading block " << std::hex << bs.tag << " in state " << (uint16_t)bs.state << " in way " << (uint16_t)bs.way << " in set " << theSet ));
      _State state(_State::char2State(bs.state));
      //int32_t way = bs.way;
      MemoryAddress tag = MemoryAddress(bs.tag);
      theMRUOrder[i] = i;
      Set<_State, _DefaultState>::theBlocks[i].tag() = tag;
      Set<_State, _DefaultState>::theBlocks[i].state() = state;
    }
    return false;
  }

  virtual bool loadTextState ( std::istream & is, int32_t theIndex, int32_t theSet, int32_t theTagShift, int32_t theSetShift ) {

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    is >> paren; // {
    if ( paren != '{' ) {
      DBG_ ( Crit, ( << "Expected '{' when loading checkpoint, read '" << paren << "' instead" ) );
      return true;
    }

    for ( int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++ ) {
      uint64_t tag;
      int32_t state;
      is >> paren >> state >> tag >> paren;
      Set<_State, _DefaultState>::theBlocks[i].tag() = MemoryAddress( (tag << theTagShift) | (theSet << theSetShift) );
      Set<_State, _DefaultState>::theBlocks[i].state() = _State::int2State(state);
      DBG_ ( Trace, ( << theIndex << "-L?: Loading block " << std::hex << tag << ", in state " << (Set<_State, _DefaultState>::theBlocks[i].state() )));
    }

    is >> paren; // }
    if ( paren != '}' ) {
      DBG_ ( Crit, ( << "Expected '}' when loading checkpoint" ) );
      return true;
    }

    // useless associativity information
    is >> paren; // <
    if ( paren != '<' ) {
      DBG_ (Crit, ( << "Expected '<' when loading checkpoint" ) );
      return false;
    }
    for ( int32_t j = 0; j < Set<_State, _DefaultState>::theAssociativity; j++ ) {
      is >> dummy;
    }
    is >> paren; // >
    if ( paren != '>' ) {
      DBG_ (Crit, ( << "Expected '>' when loading checkpoint" ) );
      return false;
    }

    return false;

  }

protected:

  inline int32_t lruListHead ( void ) {
    return theMRUOrder[0];
  }
  inline int32_t lruListTail ( void ) {
    return theMRUOrder[Set<_State, _DefaultState>::theAssociativity-1];
  }

  inline void moveToHead ( const SetIndex aBlock ) {
    int32_t i = 0;
    while ( theMRUOrder[i] != aBlock ) i++;
    while ( i > 0 ) {
      theMRUOrder[i] = theMRUOrder[i - 1];
      i--;
    }
    theMRUOrder[0] = aBlock;
  }

  inline void moveToTail ( const SetIndex aBlock ) {
    int32_t i = 0;
    while ( theMRUOrder[i] != aBlock) i++;
    while ( i < (Set<_State, _DefaultState>::theAssociativity - 1) ) {
      theMRUOrder[i] = theMRUOrder[i+1];
      i++;
    }
    theMRUOrder[Set<_State, _DefaultState>::theAssociativity - 1] = aBlock;
  }

  SetIndex * theMRUOrder;

}; // class SetLRU

template<typename _State, const _State & _DefaultState>
class SetPseudoLRU : public Set<_State, _DefaultState> {
public:
  SetPseudoLRU( const int32_t anAssociativity )
    : Set<_State, _DefaultState> ( anAssociativity ), theMRUBits(anAssociativity, ((1ULL << anAssociativity) - 1) ) {
  }

  virtual Block<_State, _DefaultState>* pickVictim() {
    DBG_Assert(false);
    // TODO: Fix this to check protected bits!!
    // First look for "invalid block", ie. block in DefaultState
    for (int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++) {
      if (Set<_State, _DefaultState>::theBlocks[i].state() == _DefaultState) {
        return &Set<_State, _DefaultState>::theBlocks[i];
      }
    }

    int32_t theBlockNum = theMRUBits.find_first();
    return Set<_State, _DefaultState>::theBlocks + theBlockNum;
  }

  virtual bool victimAvailable() {
    DBG_Assert(false);
    // First look for "invalid block", ie. block in DefaultState
    for (int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++) {
      if (Set<_State, _DefaultState>::theBlocks[i].state() == _DefaultState) {
        return true;
      }
    }

    return true;
    // TODO: Fix this to check protected bits!!
  }

  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    theMRUBits[theBlockNum] = 0;
    if (theMRUBits.none()) {
      theMRUBits.flip();
      theMRUBits[theBlockNum] = 0;
    }
  }

  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    theMRUBits[theBlockNum] = 1;
    //ALEX
    StreamBuffer<_State, _DefaultState>* theStrBuf = aBlock->associatedStreamBuffer();
    if (theStrBuf) {
        int32_t theCanceledATT = theStrBuf->invalidateStreamBuffer(aBlock->tag());
        //TODO: Somehow pass this ATT to RRPP
    }
  }

  virtual bool saveState ( std::ostream & s ) {
    return true;
  }

  virtual bool loadState ( boost::archive::binary_iarchive & ia, int32_t theIndex, int32_t theSet ) {
    return true;
  }

  virtual bool loadTextState ( std::istream & is, int32_t theIndex, int32_t theSet, int32_t theTgShift, int32_t theSetShift ) {
    return true;
  }

protected:

  // This uses one bit per bool, pretty space-efficient
  boost::dynamic_bitset<> theMRUBits;

}; // class SetPseudoLRU

template<typename _State, const _State & _DefaultState>
class StdArray : public AbstractArray<_State> {
protected:

  int32_t theCacheSize;
  int32_t theAssociativity;
  int32_t theBlockSize;
  int32_t theReplacementPolicy;

  // Masks to address portions of the cache
  int32_t setCount;
  int32_t setIndexShift;
  int32_t setIndexMask;

  int32_t theTagShift; // Used when loading Text flexpoints

  // provided by AbstractArray<_State>
  //MemoryAddress blockOffsetMask;

  //ALEX
  uint32_t theStreamBufferCount;
  uint32_t theStreamBufferDepth;

  Set<_State, _DefaultState> ** theSets;
  StreamBuffer<_State, _DefaultState> ** theStreamBuffers;
  std::list<uint32_t> canceledATTs;
  std::list<uint32_t> theAvailableStreamBuffers;

public:

  virtual ~StdArray () {}
  StdArray ( const int32_t aBlockSize, const std::list< std::pair< std::string, std::string> > &theConfiguration) {
    theBlockSize = aBlockSize;

    theStreamBufferCount = 0;
    theStreamBufferDepth = 0;

    std::list< std::pair< std::string, std::string> >::const_iterator iter = theConfiguration.begin();
    for (; iter != theConfiguration.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "size") == 0) {
        theCacheSize = strtoull(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "assoc") == 0 || strcasecmp(iter->first.c_str(), "associativity") == 0) {
        theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "repl") == 0 || strcasecmp(iter->first.c_str(), "replacement") == 0) {
        if (strcasecmp(iter->second.c_str(), "lru") == 0) {
          theReplacementPolicy = REPLACEMENT_LRU;
        } else if (strcasecmp(iter->second.c_str(), "plru") == 0
                   || strcasecmp(iter->second.c_str(), "pseudoLRU") == 0) {
          theReplacementPolicy = REPLACEMENT_PLRU;
        } else {
          DBG_Assert( false, ( << "Invalid replacement policy type " << iter->second ));
        }
      } else if (strcasecmp(iter->first.c_str(), "strBufs") == 0 || strcasecmp(iter->first.c_str(), "streamBuffers") == 0) {
          theStreamBufferCount = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "strBufDepth") == 0 || strcasecmp(iter->first.c_str(), "streamBufferDepth") == 0) {
          theStreamBufferDepth = strtol(iter->second.c_str(), NULL, 0);
      } else {
        DBG_Assert( false, ( << "Unknown configuration parameter '" << iter->first << "' while creating StdArray. Valid params are: size,assoc,repl,strBufs,strBufDepth" ));
      }
    }

    if (theStreamBufferCount) DBG_(Dev, ( << "\tThis cache has " << theStreamBufferCount << " stream buffers of " << theStreamBufferDepth << " entries for use by RRPPs."));
    init();
  }

  void init() {
    int32_t indexBits, blockOffsetBits, i;

    DBG_Assert ( theCacheSize     > 0 );
    DBG_Assert ( theAssociativity > 0 );
    DBG_Assert ( theBlockSize     > 0 );

    //  Physical address layout:
    //
    //  +---------+-------+-------------------+
    //  | Tag     | Index |    BlockOffset    |
    //  +---------+-------+-------------------+
    //                    |<- setIndexShift ->|
    //            |<----->|
    //             setIndexMask (shifted right)
    //  |<------->|
    //    tagMask
    //            |<------ tagShift --------->|

    // Set indexes and masks
    setCount = theCacheSize / theAssociativity / theBlockSize;

    DBG_Assert( (setCount & (setCount - 1)) == 0);

    indexBits       = log_base2 ( setCount );
    blockOffsetBits = log_base2 ( theBlockSize );

    AbstractArray<_State>::blockOffsetMask = MemoryAddress((1ULL << blockOffsetBits) - 1);

    setIndexShift = blockOffsetBits;
    setIndexMask  = (1 << indexBits) - 1;

    theTagShift = blockOffsetBits + indexBits;

    // Allocate the sets
    theSets = new Set<_State, _DefaultState>*[setCount];
    DBG_Assert ( theSets );

    for ( i = 0; i < setCount; i++ ) {

      switch ( theReplacementPolicy ) {
        case REPLACEMENT_LRU:
          theSets[i] = new SetLRU<_State, _DefaultState> ( theAssociativity );
          break;
        case REPLACEMENT_PLRU:
          theSets[i] = new SetPseudoLRU<_State, _DefaultState> ( theAssociativity );
          break;
        default:
          DBG_Assert ( 0 );
      };
    }

    if (theStreamBufferCount && theStreamBufferDepth) {
      theStreamBuffers = new StreamBuffer<_State, _DefaultState>*[theStreamBufferCount];
      uint32_t j;
      for (j=0; j<theStreamBufferCount; j++) {
          theStreamBuffers[j] = new StreamBuffer<_State, _DefaultState>(theStreamBufferDepth, j);
          theStreamBuffers[j]->clear();
          theAvailableStreamBuffers.push_back(j);
      }
    }
  }

  StreamBuffer<_State, _DefaultState> *lookupStrBufs(const MemoryAddress & anAddress) {
    StreamBuffer<_State, _DefaultState> *theStreamBuffer = NULL;
    uint32_t i;
    for (i=0; i<theStreamBufferCount; i++) {
        theStreamBuffer = theStreamBuffers[i]->lookupBlock(anAddress);
        if (theStreamBuffer) break;
    }
    return theStreamBuffer;
  }

  // Main array lookup function
  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > operator[] ( const MemoryAddress & anAddress ) {
    //boost::intrusive_ptr<AbstractLookupResult<_State> > ret = theSets[makeSet(anAddress)]->lookupBlock( blockAddress(anAddress) );
    //DBG_(Trace, ( << "Found block " << std::hex << blockAddress(anAddress) << " (" << anAddress  << ") in set " << makeSet(anAddress) << " in state " << ret->state() ));
      MemoryAddress myAddress = anAddress;
      if (myAddress & STR_BUF_MASK) { //ALEX - Distinguish between access that go to the Stream Buffer and normal accesses
          //boost::intrusive_ptr<AbstractLookupResult<_State> > ret = lookupStrBufs(blockAddress(anAddress));
          //TODO
          myAddress = (MemoryAddress)((uint64_t)myAddress & (STR_BUF_MASK-1));
          DBG_(RRPP_STREAM_BUFFERS, ( << "Received a lookup request from an RRPP. Marking Stream Buffer entry. Turning address 0x"
                      << std::hex << anAddress << " to 0x" << myAddress));
      }
      boost::intrusive_ptr<AbstractLookupResult<_State> > ret = theSets[makeSet(myAddress)]->lookupBlock( blockAddress(myAddress) );
      DBG_(Trace, ( << "Found block " << std::hex << blockAddress(myAddress) << " (" << myAddress  << ") in set " << makeSet(myAddress) << " in state " << ret->state() ));
    return ret;
  }

  virtual bool canAllocate( boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, const MemoryAddress & anAddress ) {
    MemoryAddress myAddress = anAddress;
    if (myAddress & STR_BUF_MASK) { //ALEX - Distinguish between access that go to the Stream Buffer and normal accesses
        myAddress = (MemoryAddress)((uint64_t)myAddress & (STR_BUF_MASK-1));
        DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP-sourced request - block was found in Invalid state, about to allocate. Turning address 0x"
              << std::hex << anAddress << " to 0x" << myAddress));
    }
    StdLookupResult<_State, _DefaultState> *std_lookup = dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
    DBG_Assert(std_lookup != nullptr);

    return std_lookup->theSet->canAllocate(std_lookup, blockAddress(anAddress));

  }
  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > allocate( boost::intrusive_ptr<AbstractLookupResult<_State> > lookup,
      const MemoryAddress & anAddress ) {
    MemoryAddress myAddress = anAddress;
    if (myAddress & STR_BUF_MASK) { //ALEX - Distinguish between access that go to the Stream Buffer and normal accesses
        myAddress = (MemoryAddress)((uint64_t)myAddress & (STR_BUF_MASK-1));
        DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP-sourced request - allocating block in cache. Turning address 0x"
              << std::hex << anAddress << " to 0x" << myAddress));
    }
    StdLookupResult<_State, _DefaultState> *std_lookup = dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
    DBG_Assert(std_lookup != nullptr);

    boost::intrusive_ptr<AbstractLookupResult<_State> > theAllocatedBlock = std_lookup->theSet->allocate(std_lookup, blockAddress(myAddress));

    if (myAddress != anAddress) {
      StreamBuffer<_State, _DefaultState> *theMatchingStrBuf = lookupStrBufs(std_lookup->theBlock->tag());
      DBG_Assert(theMatchingStrBuf);
      theMatchingStrBuf->allocateEntry(std_lookup->theBlock);
    }
 
    return theAllocatedBlock;
  }

  virtual void recordAccess(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup) {
    StdLookupResult<_State, _DefaultState> *std_lookup = dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
    DBG_Assert(std_lookup != nullptr);
    DBG_Assert( std_lookup->valid() );

    std_lookup->theSet->recordAccess(std_lookup->theBlock);
  }

  virtual void invalidateBlock(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup) {
    StdLookupResult<_State, _DefaultState> *std_lookup = dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
    DBG_Assert(std_lookup != nullptr);
    DBG_Assert( std_lookup->valid() );

    std_lookup->theSet->invalidateBlock(std_lookup->theBlock);
  }

  virtual std::pair<_State, MemoryAddress> getPreemptiveEviction() {
    return std::make_pair(_DefaultState, MemoryAddress(0));
  }

  // Checkpoint reading/writing functions
  virtual bool saveState ( std::ostream & s ) {

    // NOT IMPLEMENTED!
    return true;
  }

  virtual bool loadState ( std::istream & s, int32_t theIndex, bool text ) {

    if (text) {
      for ( int32_t i = 0; i < setCount; i++ ) {
        if ( theSets[i]->loadTextState ( s, theIndex, i, theTagShift, setIndexShift ) ) {
          DBG_ ( Crit, ( << " Error loading state for set: line number " << i ) );
          return true; // true == error
        }
      }
    } else {
      boost::archive::binary_iarchive ia(s);

      uint64_t set_count = 0;
      uint32_t associativity = 0;

      ia >> set_count;
      ia >> associativity;

      // ustiugov: remove this FIXME and bring back assertions
      if ( (set_count != (uint64_t)setCount) || (associativity != (uint64_t)theAssociativity) )
          return false;
      //DBG_Assert( set_count == (uint64_t)setCount, ( << "Error loading cache state. Flexpoint contains " << set_count << " sets but simulator configured for " << setCount << " sets." ));
      //DBG_Assert( associativity == (uint64_t)theAssociativity, ( << "Error loading cache state. Flexpoint contains " << associativity << "-way sets but simulator configured for " << theAssociativity << "-way sets." ));

      for ( int32_t i = 0; i < setCount; i++ ) {
        if ( theSets[i]->loadState ( ia, theIndex, i ) ) {
          DBG_ ( Crit, ( << " Error loading state for set: line number " << i ) );
          return true; // true == error
        }
      }
    }
    return false; // false == no errors
  }

  // Addressing helper functions
  MemoryAddress blockAddress ( MemoryAddress const & anAddress ) const {
    return MemoryAddress ( anAddress & ~(AbstractArray<_State>::blockOffsetMask) );
  }

  BlockOffset blockOffset ( MemoryAddress const & anAddress ) const {
    return BlockOffset ( anAddress & AbstractArray<_State>::blockOffsetMask );
  }

  SetIndex makeSet ( const MemoryAddress & anAddress ) const {
    return ((anAddress >> setIndexShift) & setIndexMask);
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) const {
    return (makeSet(a) == makeSet(b));
  }

  virtual std::list<MemoryAddress>  getSetTags(MemoryAddress addr) {
    return theSets[makeSet(addr)]->getTags();
  }

  virtual std::function<bool (MemoryAddress a, MemoryAddress b)> setCompareFn() const {
    //return [this](auto a, auto b){ return this->sameSet(a, b); };//std::bind( &StdArray<_State, _DefaultState>::sameSet, *this, _1, _2);
    return boost::bind( &StdArray<_State, _DefaultState>::sameSet, *this, _1, _2);
  }

  virtual uint64_t getSet(MemoryAddress const & addr) const {
    return (uint64_t)makeSet(addr);
  }

  virtual int32_t requestsPerSet() const {
    return theAssociativity;
  }

//ALEX
  StreamBuffer<_State, _DefaultState> *findStreamBufferByATT(uint32_t anATTidx) {
    uint32_t i;
    for (i=0; i<theStreamBufferCount; i++) {
      if (theStreamBuffers[i]->getATTidx() == (int)anATTidx) return (theStreamBuffers[i]);
    }
    return NULL;
  }

  bool streamBufferAvailable() {
    DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP checking StreamBuffer availability: " << !theAvailableStreamBuffers.empty() ));
    return (!theAvailableStreamBuffers.empty());
  }

  void allocateStreamBuffer(uint32_t anATTidx, uint64_t anAddress, uint32_t aLength) {
    DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP allocating StreamBuffer for ATTidx " << anATTidx ));
    #ifdef STREAM_BUFFER_DEBUG
      dumpStreamBuffers();
    #endif
    uint32_t theStrBufid = theAvailableStreamBuffers.front();
    theAvailableStreamBuffers.pop_front();
    theStreamBuffers[theStrBufid]->reserveStreamBuffer(anATTidx, anAddress, aLength);
  }

  void freeStreamBuffer(uint32_t anATTidx) {
    DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP freeing StreamBuffer that was allocated for ATTidx " << anATTidx ));
    StreamBuffer<_State, _DefaultState> *theStreamBuffer = findStreamBufferByATT(anATTidx);
    DBG_Assert(theStreamBuffer);
    theAvailableStreamBuffers.push_back(theStreamBuffer->getBufferID());
    theStreamBuffer->clear();
  }

  bool allocateStreamBufferSlot(uint32_t anATTidx) {
    StreamBuffer<_State, _DefaultState> *theStreamBuffer = findStreamBufferByATT(anATTidx);
    DBG_Assert(theStreamBuffer);
    DBG_(RRPP_STREAM_BUFFERS, ( << "RRPP checking slot availability in StreamBuffer allocated for ATTidx " << anATTidx << ". Remaining slots: " << theStreamBuffer->slotsAvailable()));
    #ifdef STREAM_BUFFER_DEBUG
      dumpStreamBuffers();
    #endif
    if (theStreamBuffer->slotsAvailable()) {
        theStreamBuffer->reserveEntry();
        return true;
    }
    return false;
  }

  std::list<uint32_t> SABReAtomicityCheck() {
    std::list<uint32_t> canceledATTlist;
    while (!canceledATTs.empty()) {
      canceledATTlist.push_back(canceledATTs.front());
      canceledATTs.pop_front();
    }
    return canceledATTlist;
  }

  void dumpStreamBuffers() {
    uint32_t i, j;
    DBG_(Dev, ( << "This cache has " << theStreamBufferCount << " Stream Buffers with a depth of " << theStreamBufferDepth));
    for (i=0; i<theStreamBufferCount; i++) {
      DBG_(Dev, ( << "\nStream Buffer " << theStreamBuffers[i]->getBufferID() << " is associated to ATTidx " << theStreamBuffers[i]->getATTidx()
                  << " and its current index is " << theStreamBuffers[i]->getIndex()));
      for (j=0; j<theStreamBufferDepth; j++) {
        DBG_(Dev, ( << "\tEntry " << j << " - Tag: " << theStreamBuffers[i]->theSBEntries[j].getTag() << ", valid: " << theStreamBuffers[i]->theSBEntries[j].getValid()));
      }
    }
  }

}; // class StdArray

};  // namespace nCache

#endif /* _CACHE_ARRAY_HPP */
