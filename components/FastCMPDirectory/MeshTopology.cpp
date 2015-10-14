#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/Utility.hpp>

#include <cmath>

using namespace Flexus::SharedTypes;

namespace nFastCMPDirectory {

class MeshTopology : public AbstractTopology {
public:
  MeshTopology() { };

  virtual std::list<int> orderSnoops(SharingVector sharers, int32_t src, bool optimize3hop, int32_t final_dest, bool multicast) {
    std::list<int> ret = sharers.toList();
    if (multicast || ret.size() == 1) {
      return ret;
    }
    std::vector<std::pair<int, int> > distances;
    std::list<int>::iterator iter;
    for (iter = ret.begin(); iter != ret.end(); iter++) {
      if (optimize3hop) {
        distances.push_back(std::make_pair((distance(src, *iter) + distance(*iter, final_dest)), *iter ) );
      } else {
        distances.push_back(std::make_pair(distance(src, *iter), *iter ) );
      }
    }
    std::sort(distances.begin(), distances.end());
    ret.clear();
    std::transform(distances.begin(), distances.end(), ret.begin(), back_inserter(ret), ((&_1) ->* &std::pair<int, int>::second) );
    return ret;
  };

  virtual int32_t memoryToIndex(PhysicalMemoryAddress address) {
    return theMemControllerMap[mem_controller_index(address)];
  };

  virtual int32_t addBW(int32_t src, int32_t dest) {
    std::pair<int, int> a, b, c;
    int32_t hop_count = 0;
    a.first = mesh_row(src);
    a.second = mesh_col(src);
    b.first = mesh_row(dest);
    b.second = mesh_col(dest);

    c = a;
    // X-Y routing, get to the right column first
    int32_t diff = ((b.second > a.second) ? 1 : -1);
    while (c.second != b.second) {
      c.second += diff;
      int32_t link_index = getLinkIndex(a, c);
      recordLinkUsage(link_index);
      hop_count++;
      a = c;
    }
    // We're at the right column, not move to the right row
    diff = ((b.first > a.first) ? 1 : -1);
    while (c.first != b.first) {
      c.first += diff;
      int32_t link_index = getLinkIndex(a, c);
      recordLinkUsage(link_index);
      hop_count++;
      a = c;
    }
    return hop_count;
  };

  virtual int32_t addBW(int32_t src, std::list<int> &dest, bool include_replies = true) {
    std::vector<bool> links_used(theNumLinks, false);
    std::pair<int, int> a, b, c;
    std::list<int>::iterator cur_dest = dest.begin();
    int32_t max_hop_count = 0;
    for (; cur_dest != dest.end(); cur_dest++) {
      int32_t hop_count = 0;
      a.first = mesh_row(src);
      a.second = mesh_col(src);
      b.first = mesh_row(*cur_dest);
      b.second = mesh_col(*cur_dest);

      c = a;
      // X-Y routing, get to the right column first
      int32_t diff = ((b.second > a.second) ? 1 : -1);
      while (c.second != b.second) {
        c.second += diff;
        int32_t link_index = getLinkIndex(a, c);
        // Use each link only once for request
        if (!links_used[link_index]) {
          recordLinkUsage(link_index);
          links_used[link_index] = true;
        }
        hop_count++;
        a = c;
      }
      // We're at the right column, not move to the right row
      diff = ((b.first > a.first) ? 1 : -1);
      while (c.first != b.first) {
        c.first += diff;
        int32_t link_index = getLinkIndex(a, c);
        // Use each link only once for request
        if (!links_used[link_index]) {
          recordLinkUsage(link_index);
          links_used[link_index] = true;
        }
        hop_count++;
        a = c;
      }

      // Now go back to the src for the reply
      b.first = mesh_row(src);
      b.second = mesh_col(src);
      // X-Y routing, get to the right column first
      diff = ((b.second > a.second) ? 1 : -1);
      while (c.second != b.second) {
        c.second += diff;
        int32_t link_index = getLinkIndex(a, c);
        recordLinkUsage(link_index);
        a = c;
      }
      // We're at the right column, not move to the right row
      diff = ((b.first > a.first) ? 1 : -1);
      while (c.first != b.first) {
        c.first += diff;
        int32_t link_index = getLinkIndex(a, c);
        recordLinkUsage(link_index);
        a = c;
      }

      if (hop_count > max_hop_count) {
        max_hop_count = hop_count;
      }
    }
    return 2 * max_hop_count;
  };

  virtual void initialize(const std::string & aName) {

    // Set shifts & masks for directory & mem controller indexing

    theMeshWidth = (uint32_t)std::ceil(std::sqrt(theNumCores));
    theMeshHeight = (uint32_t)(theNumCores / theMeshWidth);

    DBG_Assert( (theMeshWidth * theMeshHeight) == theNumCores );
    DBG_Assert( theMeshWidth == theMeshHeight ); // Some of the code below assumes a square mesh

    theVerticalLinksPerRow = theMeshWidth - 1;
    theTotalLinksPerRow = (theMeshWidth << 1) - 1;
    theNumLinks = theTotalLinksPerRow * (theMeshHeight - 1) + theVerticalLinksPerRow;

    DBG_( Dev, ( << "Creating 2-D Mesh Topology: " << theVerticalLinksPerRow << " V links per row, " << theTotalLinksPerRow << " T links per row, " << theNumLinks << " Total Links, Mesh Width = " << theMeshWidth ));

    DBG_Assert( ((theMeshWidth - 1) & theMeshWidth) == 0 );
    DBG_Assert( (theNumMemControllers & (theNumMemControllers - 1)) == 0); // Make sure it's a power of 2
    DBG_Assert( (theMemControllerInterleaving & (theMemControllerInterleaving - 1)) == 0); // Make sure it's a power of 2

    theMemControllerShift = log_base2(theMemControllerInterleaving);
    theMemControllerMask = theNumMemControllers - 1;

#if 0
    // Be generic is HARD, I'm LAZY, so let's make this only work with 16 nodes and 4 memcontrollers
    DBG_Assert(theNumCores == 16);
    DBG_Assert(theNumMemControllers == 4);
#endif

    theMemControllerMap = new int[theNumMemControllers];

    // Need to map directory index to node index
    // Edges -> spaced evenly on the edge, Sides -> centered on two sides, Tiles -> centered in the grid,
    if (theMemControllerLocation == "Edges") {
      std::list<int> nodes;
      // Add nodes from the top (except the last one)
      for (int32_t i = 0; i < (theMeshWidth - 1); i++) {
        nodes.push_back(i);
      }
      // right side
      for (int32_t i = 1; i < (theMeshWidth - 1); i++) {
        nodes.push_back((i + 1)*theMeshWidth - 1);
      }
      // bottom side
      for (int32_t i = theMeshWidth - 2; i > 0; i--) {
        nodes.push_back(theMeshWidth*(theMeshWidth - 1) + i);
      }
      // left side
      for (int32_t i = theMeshWidth - 2; i > 0; i--) {
        nodes.push_back(i * theMeshWidth);
      }

      int32_t theSkipWidth = (int)std::ceil(theMeshWidth / ((theNumMemControllers / 4) + 1.0));
      DBG_(Dev, ( << "SkipWidth = " << theSkipWidth ));
      for (int32_t i = 0; i < theNumMemControllers; i++) {
        for (int32_t j = 1; j < theSkipWidth; j++) {
          nodes.pop_front();
        }
        theMemControllerMap[i] = nodes.front();
        nodes.pop_front();
      }
    } else if (theMemControllerLocation == "Sides") {
      // Make a list of nodes on alternating sides.
      std::list<int> nodes;
      for (int32_t i = 0; i < theMeshWidth; i++) {
        nodes.push_back(i * theMeshWidth);
        nodes.push_back((i + 1)*theMeshWidth - 1);
      }

      // Half on one side, half on the other, centered in the middle of each side
      // Centered -> remove ((Height-NumPerSide)/2) from start of each side
      for (int32_t i = 0; i < (theMeshHeight - (theNumMemControllers >> 1)); i++) {
        nodes.pop_front();
      }

      for (int32_t i = 0; i < theNumMemControllers; i++) {
        theMemControllerMap[i] = nodes.front();
        nodes.pop_front();
      }

    } else if (theMemControllerLocation == "Tiles") {
      if ((theNumMemControllers = 4) && (theNumCores == 16)) {
        theMemControllerMap[0] = 5;
        theMemControllerMap[1] = 6;
        theMemControllerMap[2] = 9;
        theMemControllerMap[3] = 10;
      } else if ((theNumMemControllers = 8) && (theNumCores == 64)) {
        theMemControllerMap[0] = 19;
        theMemControllerMap[1] = 20;
        theMemControllerMap[2] = 29;
        theMemControllerMap[3] = 37;
        theMemControllerMap[4] = 44;
        theMemControllerMap[5] = 43;
        theMemControllerMap[6] = 34;
        theMemControllerMap[7] = 26;
      } else {
        DBG_Assert(false);
      }

    } else {
      DBG_Assert( false, ( << "Unknown MemControllerLocation '" << theMemControllerLocation << "'" ));
    }

    for (int32_t i = 0; i < theNumMemControllers; i++) {
      DBG_(Dev, ( << "MemControllerMap[" << i << "] = " << theMemControllerMap[i] ));
    }

    AbstractTopology::initialize(aName);
  };

  static AbstractTopology * createInstance(std::list< std::pair<std::string, std::string> > &arg_list) {
    MeshTopology * topology = new MeshTopology();
    topology->processGenericOptions(arg_list);
    std::list< std::pair<string, string> >::iterator iter = arg_list.begin();
    for (; iter != arg_list.end(); iter++) {
      // Any Additional Arguments are parsed here.
    }
    return topology;
  };

  static const std::string name;

private:
  uint64_t theMemControllerMask;
  int theMemControllerShift;
  int * theMemControllerMap;

  int32_t theMeshWidth;
  int32_t theMeshHeight;
  int32_t theTotalLinksPerRow;
  int32_t theVerticalLinksPerRow;

  inline uint64_t mem_controller_index(PhysicalMemoryAddress addr) {
    return (( (addr) >> theMemControllerShift ) & theMemControllerMask);
  }

  inline int32_t mesh_row(int32_t index) {
    return ((int)(std::floor( (index) / theMeshWidth )));
  }

  inline int32_t mesh_col(int32_t index) {
    return ((int)(std::floor( (index) % theMeshWidth )));
  }

  inline int32_t distance(int32_t a, int32_t b) {
    return (ABS(mesh_row(a) - mesh_row(b)) + ABS(mesh_col(a) - mesh_col(b)) );
  }

  inline int32_t getLinkIndex(const std::pair<int, int> &a, const std::pair<int, int> &b) {
    return (MIN(a.second, b.second) + theTotalLinksPerRow * MIN(a.first, b.first) + theVerticalLinksPerRow * ABS(a.first - b.first));
  }

};

REGISTER_TOPOLOGY_TYPE(MeshTopology, "Mesh");

};
