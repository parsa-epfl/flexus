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
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <core/debug/debug.hpp>

#include <components/FastCMPCache/AbstractDirectory.hpp>
#include <components/FastCMPCache/AbstractProtocol.hpp>
#include <components/FastCMPCache/BlockDirectoryEntry.hpp>
#include <components/FastCMPCache/SharingVector.hpp>

#include <tuple>

#include <list>

#include <components/CommonQEMU/Serializers.hpp>
#include <components/CommonQEMU/Util.hpp>
using nCommonSerializers::StdDirEntrySerializer;
using nCommonUtil::log_base2;

namespace nFastCMPCache {

typedef BlockDirectoryEntry StandardDirectoryEntry;

class StandardDirectory : public AbstractDirectory {
private:
  int32_t theNumSets;
  int32_t theAssociativity;

  bool theSkewSet;
  bool theSpecialInterleaving;
  int32_t theSetShift;
  int32_t theSetMask;
  int32_t theSkewShift;

  enum ReplPolicy { LRURepl, MinSharers } theReplPolicy;

  std::vector<std::vector<StandardDirectoryEntry>> theDirectory;

  StandardDirectory() : AbstractDirectory(), theReplPolicy(LRURepl){};

  virtual void initialize(const std::string &aName) {

    AbstractDirectory::initialize(aName);

    DBG_Assert(theNumSets > 0);
    DBG_Assert(theAssociativity > 0);

    theDirectory.resize(theNumSets, std::vector<StandardDirectoryEntry>(theAssociativity,
                                                                        StandardDirectoryEntry()));

    theSetMask = theNumSets - 1;

    theSetShift = log_base2(theBlockSize);

    // The Physical address has 34 usable bits (33,32 = vm index)
    theSkewShift = 34 - log_base2(theNumSets);

    DBG_(Dev, (<< "Initialized Directory with " << theNumSets << " x " << theAssociativity
               << "-way sets"));
    DBG_(Dev,
         (<< "\tSetMask = " << std::hex << theSetMask << ", SetShift = " << std::dec << theSetShift
          << ", SkewShift = " << theSkewShift << ", SkewSet = " << std::boolalpha << theSkewSet));
  }

  inline int32_t get_set(PhysicalMemoryAddress addr) {

    if (theSpecialInterleaving) {
      uint64_t index = addr >> theSetShift;
      uint64_t bank =
          (index & 3) | ((index & 4) << 1) | ((index & 64) >> 4) | ((index & 0x180) >> 3);
      uint64_t set = (index & ~0x1FF) | ((index & 0x38) << 3);
      if (theSkewSet) {
        return ((set ^ ((addr >> theSkewShift) & ~0x3F)) | bank) & theSetMask;
      } else {
        return (set | bank) & theSetMask;
      }
    }

    if (theSkewSet) {
      return (((uint64_t)addr >> theSetShift) ^ ((uint64_t)addr >> theSkewShift)) & theSetMask;
    } else {
      return ((uint64_t)addr >> theSetShift) & theSetMask;
    }
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // StandardDirectoryEntry *my_entry =
    // dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry *my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == nullptr) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index);
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry,
                                  PhysicalMemoryAddress address) {
    // StandardDirectoryEntry *my_entry =
    // dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry *my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    DBG_Assert(my_entry != nullptr);
    my_entry->addSharer(index);
    my_entry->makeExclusive(index);
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry,
                           PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry,
                            PhysicalMemoryAddress address) {
    // StandardDirectoryEntry *my_entry =
    // dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry *my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == nullptr) {
      return;
    }
    my_entry->removeSharer(index);
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry,
                                   PhysicalMemoryAddress address) {
    // StandardDirectoryEntry *my_entry =
    // dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry *my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == nullptr) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index);
  }

  StandardDirectoryEntry *findOrCreateEntry(PhysicalMemoryAddress addr, bool make_lru = true) {
    int32_t min_sharers = INT_MAX;
    int32_t min_way = -1;
    int32_t set = get_set(addr);
    for (int32_t way = 0; way < theAssociativity; way++) {
      if (theDirectory[set][way].tag() == addr) {
        if (make_lru) {
          std::vector<StandardDirectoryEntry>::iterator iter = theDirectory[set].begin();
          std::rotate(iter, iter + way, iter + way + 1);
          way = 0;
        }
        return &(theDirectory[set][way]);
      } else if (theReplPolicy == LRURepl && theDirectory[set][way].state() == ZeroSharers) {
        min_way = way;
      } else if (theReplPolicy == MinSharers &&
                 theDirectory[set][way].sharers().countSharers() < min_sharers) {
        min_way = way;
        min_sharers = theDirectory[set][way].sharers().countSharers();
      }
    }

    if (min_way == -1) {
      min_way = theAssociativity - 1;
    }
    if (make_lru) {
      std::vector<StandardDirectoryEntry>::iterator iter = theDirectory[set].begin();
      std::rotate(iter, iter + min_way, iter + min_way + 1);
      min_way = 0;
    }

    return &(theDirectory[set][min_way]);
  }

public:
  virtual void updateLRU(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // TODO: Update LRU Information
    // Also need to change replacement policy
  }

  virtual std::tuple<SharingVector, SharingState, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type,
         std::list<std::function<void(void)>> &xtra_actions) {

    StandardDirectoryEntry* entry = findOrCreateEntry(address, !MemoryMessage::isEvictType(req_type));
    DBG_Assert(entry != nullptr);

    if (entry->tag() != address) {
      // We're evicting an existing entry

      auto tag  = entry->tag();
      auto sharers = entry->sharers();

      xtra_actions.push_back(
        [tag, sharers, this]() { theInvalidateAction(tag, sharers); }
      );

      entry->reset(address);
    }

    BlockEntryWrapper_p wrapper(new BlockEntryWrapper(*entry));

    return std::tie(entry->sharers(), entry->state(), wrapper);
  }

  virtual std::tuple<SharingVector, SharingState, AbstractEntry_p, bool>
  snoopLookup(int32_t index, PhysicalMemoryAddress address, MMType req_type) {

    StandardDirectoryEntry *entry = findOrCreateEntry(address, false);
    DBG_Assert(entry != nullptr);

    bool valid = true;
    if (entry->tag() != address) {
      SharingVector sharers;
      SharingState state = ZeroSharers;
      BlockEntryWrapper_p wrapper;
      valid = false;
      return std::tie(sharers, state, wrapper, valid);
    }

    BlockEntryWrapper_p wrapper(new BlockEntryWrapper(*entry));

    return std::tie(entry->sharers(), entry->state(), wrapper, valid);
  }
  void saveState(std::ostream &s, const std::string &aDirName) {
    boost::archive::binary_oarchive oa(s);

    uint64_t set_count = theNumSets;
    uint32_t associativity = theAssociativity;

    oa << set_count;
    oa << associativity;

    StdDirEntrySerializer serializer;
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        serializer = theDirectory[set][way].getSerializer();
        DBG_(Trace, (<< "Directory saving block " << serializer));
        oa << serializer;
      }
    }
  }

  bool loadState(std::istream &s, const std::string &aDirName) {
    boost::archive::binary_iarchive ia(s);

    uint64_t set_count = 0;
    uint32_t associativity = 0;

    ia >> set_count;
    ia >> associativity;

    DBG_Assert(set_count == (uint64_t)theNumSets,
               (<< "Error loading directory state. Flexpoint contains " << set_count
                << " sets but simulator configured for " << theNumSets << " sets."));
    DBG_Assert(associativity == (uint64_t)theAssociativity,
               (<< "Error loading directory state. Flexpoint contains " << associativity
                << "-way sets but simulator configured for " << theAssociativity << "-way sets."));

    StdDirEntrySerializer serializer;
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        ia >> serializer;
        theDirectory[set][way] = serializer;
      }
    }
    return true;
  }

  static AbstractDirectory *createInstance(std::list<std::pair<std::string, std::string>> &args) {
    StandardDirectory *directory = new StandardDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string>>::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "sets") == 0) {
        directory->theNumSets = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "assoc") == 0) {
        directory->theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "repl") == 0) {
        if (strcasecmp(iter->second.c_str(), "lru") == 0) {
          directory->theReplPolicy = LRURepl;
        } else if (strcasecmp(iter->second.c_str(), "min_sharers") == 0) {
          directory->theReplPolicy = MinSharers;
        } else {
          DBG_Assert(false, (<< "Unknown Replacement policty: '" << iter->second << "'"));
        }
      } else if (strcasecmp(iter->first.c_str(), "skew") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSkewSet = true;
        } else {
          directory->theSkewSet = false;
        }
      } else if (strcasecmp(iter->first.c_str(), "special") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSpecialInterleaving = true;
        } else {
          directory->theSpecialInterleaving = false;
        }
      }
    }

    return directory;
  }

  static const std::string name;
};

REGISTER_DIRECTORY_TYPE(StandardDirectory, "Standard");

}; // namespace nFastCMPCache
