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

#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

#define DIMS 2

int32_t numNodes = 0;
int32_t nodesPerDim[DIMS] = {0};

bool isTorus = false;
bool isAdaptive = false;

char *outFilename;
ofstream outFile;

bool processArguments(int32_t argc, char **argv);
bool generateSwitches(void);

bool generateMeshTopology(int32_t nodeId);
bool generateTorusTopology(int32_t nodeId);

bool generateDeadlockFreeMeshRoute(int32_t currNode, int32_t destNode);

bool generateDeadlockFreeTorusRoute(int32_t currNode, int32_t destNode);

enum Direction { NORTH, SOUTH, EAST, WEST, LOCAL };

int32_t main(int32_t argc, char **argv) {

  int i, j;

  if (argc != 5) {
    cerr << "Usage: " << argv[0] << " numnodes -t|-m -d|-a outputfile" << endl;
    cerr << "   -t specifies a 2D torus" << endl;
    cerr << "   -m specifies a 2D mesh" << endl;
    cerr << "   -d specifies dimension ordered routing" << endl;
    cerr << "   -a specifies adaptive routing tables (not implemented)" << endl;

    return 1;
  }

  if (processArguments(argc, argv))
    return 1;

  // Generate the boilerplate parameters
  outFile << "# Boilerplate stuff" << endl;
  outFile << "ChannelLatency 100" << endl;
  outFile << "ChannelLatencyData 32" << endl;
  outFile << "ChannelLatencyControl 1" << endl;
  outFile << "LocalChannelLatencyDivider 8" << endl;
  outFile << "SwitchInputBuffers 5" << endl;
  outFile << "SwitchOutputBuffers 1" << endl;
  outFile << "SwitchInternalBuffersPerVC 1" << endl;
  outFile << endl;

  // Output all of the node->switch connections
  if (generateSwitches())
    return true;

  // Make topology
  if (isTorus) {
    outFile << endl << "# Topology for a " << numNodes << " node TORUS" << endl;
  } else {
    outFile << endl << "# Topology for a " << numNodes << " node MESH" << endl;
  }

  for (i = 0; i < numNodes; i++) {
    if (isTorus) {
      if (generateTorusTopology(i))
        return 1;
    } else {
      if (generateMeshTopology(i))
        return 1;
    }
  }

  outFile << endl << "# Deadlock-free routing tables" << endl;

  // For each switch, generate a routing table to each destination
  for (i = 0; i < numNodes; i++) {

    outFile << endl << "# Switch " << i << " -> *" << endl;

    // For each destination
    for (j = 0; j < numNodes; j++) {
      if (isTorus) {
        if (generateDeadlockFreeTorusRoute(i, j))
          return 1;
      } else {
        if (generateDeadlockFreeMeshRoute(i, j))
          return 1;
      }
    }
  }

  outFile.close();

  return 0;
}

bool processArguments(int32_t argc, char **argv) {
  int i;

  numNodes = atoi(argv[1]);

  nodesPerDim[0] = (int)sqrt((float)numNodes);

  // Get this to a power of two
  while (nodesPerDim[0] & (nodesPerDim[0] - 1))
    nodesPerDim[0]--;

  nodesPerDim[1] = numNodes / nodesPerDim[0];

  if (numNodes <= 0 || (numNodes & (numNodes - 1))) {
    cerr << "NumNodes must be greater than zero and a power of two" << endl;
    return true;
  }

  for (i = 0; i < DIMS; i++) {
    cout << nodesPerDim[i] << " nodes per dimension " << i << endl;
  }

  if (!strcasecmp(argv[2], "-t")) {
    isTorus = true;
  } else if (!strcasecmp(argv[2], "-m")) {
    isTorus = false;
  } else {
    cerr << "Unexpected argument: " << argv[2] << endl;
    return true;
  }

  if (!strcasecmp(argv[3], "-d")) {
    isAdaptive = false;
  } else if (!strcasecmp(argv[3], "-a")) {
    isAdaptive = true;
    cerr << "ERROR: adaptive routing not supported yet" << endl;
    return true;
  } else {
    cerr << "Unexpected argument: " << argv[3] << endl;
    return true;
  }

  outFilename = argv[4];

  outFile.open(outFilename);
  if (!outFile.good()) {
    cerr << "ERROR opening output file: " << outFilename << endl;
    return true;
  }

  return false;
}

bool generateSwitches(void) {
  int i;

  outFile << "# Basic Switch/Node connections" << endl;
  outFile << "NumNodes " << numNodes << endl;
  outFile << "NumSwitches " << numNodes << endl;
  outFile << "SwitchPorts   5" << endl;
  outFile << "SwitchBandwidth 4" << endl;
  outFile << endl;

  for (i = 0; i < numNodes; i++) {
    outFile << "Top Node " << i << " -> Switch " << i << ":0" << endl;
  }

  return false;
}

int32_t getXCoord(int32_t nodeId) {
  return (nodeId % nodesPerDim[0]);
}

int32_t getYCoord(int32_t nodeId) {
  return (nodeId / nodesPerDim[0]);
}

int32_t getNodeIdCoord(int32_t x, int32_t y) {
  int id;

  id = (x + (y * nodesPerDim[0]));

  if (id >= numNodes) {
    cerr << "ERROR: node coordinates out of bounds: " << x << ", " << y << endl;
    exit(1);
  }

  return id;
}

int32_t getNodeIdOffset(int32_t nodeId, Direction dir) {
  int x, y;

  x = getXCoord(nodeId);
  y = getYCoord(nodeId);

  switch (dir) {
  case NORTH:
    y--;
    break;
  case SOUTH:
    y++;
    break;
  case EAST:
    x++;
    break;
  case WEST:
    x--;
    break;

  default:
    cerr << "Invalid direction" << dir << endl;
    exit(1);
  }

  if (isTorus) {

    if (x < 0)
      x = nodesPerDim[0] - 1;

    if (x >= nodesPerDim[0])
      x = 0;

    if (y < 0)
      y = nodesPerDim[1] - 1;

    if (y >= nodesPerDim[1])
      y = 0;

  } else {

    // Invalid offset for a mesh
    if (x < 0 || y < 0 || x >= nodesPerDim[0] || y >= nodesPerDim[1]) {

      cerr << "ERROR: invalid offset for a mesh!" << endl;
      return -1;
    }
  }

  return getNodeIdCoord(x, y);
}

ostream &operator<<(ostream &out, const Direction dir) {

  switch (dir) {
  case NORTH:
    out << "2";
    break;
  case SOUTH:
    out << "4";
    break;
  case EAST:
    out << "3";
    break;
  case WEST:
    out << "1";
    break;
  case LOCAL:
    out << "0";
    break;
  };

  return out;
}

bool writeS2S(const int32_t node1, const Direction dir1, const int32_t node2,
              const Direction dir2) {

  outFile << "Top Switch " << node1 << ":" << dir1 << " -> Switch " << node2 << ":" << dir2 << endl;

  return false;
}

/* A switch looks like this:
 *  (port numbers on inside, port 0 is the local CPU)
 *
 *               N
 *               |
 *        +-------------+
 *        |      2      |
 *        |             |
 *   W -- | 1    0    3 | -- E
 *        |             |
 *        |      4      |
 *        +-------------+
 *               |
 *               S
 */
bool generateMeshTopology(int32_t nodeId) {
  int x, y;

  x = getXCoord(nodeId);
  y = getYCoord(nodeId);

  // East
  if (x != nodesPerDim[0] - 1)
    writeS2S(getNodeIdCoord(x, y), EAST, getNodeIdCoord(x + 1, y), WEST);

  // South
  if (y != nodesPerDim[1] - 1)
    writeS2S(getNodeIdCoord(x, y), SOUTH, getNodeIdCoord(x, y + 1), NORTH);

  return false;
}

bool generateTorusTopology(int32_t nodeId) {
  int x, y;

  x = getXCoord(nodeId);
  y = getYCoord(nodeId);

  // East
  writeS2S(getNodeIdCoord(x, y), EAST, getNodeIdCoord((x + 1) % nodesPerDim[0], y), WEST);

  // South
  writeS2S(getNodeIdCoord(x, y), SOUTH, getNodeIdCoord(x, (y + 1) % nodesPerDim[1]), NORTH);

  return false;
}

bool writeBasicRoute(int32_t currNode, int32_t destNode, Direction outPort, int32_t outVC) {
  outFile << "Route Switch " << currNode << " -> " << destNode << " { " << outPort << ":" << outVC
          << " } " << endl;
  return false;
}

#define ABS(X) ((X) > 0 ? (X) : -(X))

bool generateDeadlockFreeTorusRoute(int32_t currNode, int32_t destNode) {
  int xoff, yoff;

  // Trivial case, output to port 0, VC 0
  if (currNode == destNode) {
    return writeBasicRoute(currNode, destNode, LOCAL, 0);
  }

  xoff = getXCoord(destNode) - getXCoord(currNode);

  yoff = getYCoord(destNode) - getYCoord(currNode);

  if (xoff != 0) {

    if (xoff > 0 && xoff < (nodesPerDim[0] / 2) || xoff < (-nodesPerDim[0] / 2)) {
      // Go EAST
      return writeBasicRoute(currNode, destNode, EAST, getXCoord(destNode) > getXCoord(currNode));
    } else {
      // Go WEST
      return writeBasicRoute(currNode, destNode, WEST, getXCoord(destNode) > getXCoord(currNode));
    }
  }

  if (yoff > 0 && yoff < (nodesPerDim[1] / 2) || yoff < (-nodesPerDim[1] / 2)) {
    // Go SOUTH
    return writeBasicRoute(currNode, destNode, SOUTH, getYCoord(destNode) > getYCoord(currNode));
  } else {
    // Go NORTH
    return writeBasicRoute(currNode, destNode, NORTH, getYCoord(destNode) > getYCoord(currNode));
  }

  return false;
}

bool generateDeadlockFreeMeshRoute(int32_t currNode, int32_t destNode) {
  int xoff, yoff;

  xoff = getXCoord(destNode) - getXCoord(currNode);

  yoff = getYCoord(destNode) - getYCoord(currNode);

  // Trivial case, output to port 0, VC 0
  if (currNode == destNode) {
    return writeBasicRoute(currNode, destNode, LOCAL, 0);
  }

  if (xoff < 0) {
    return writeBasicRoute(currNode, destNode, WEST, 0);
  }

  if (xoff > 0) {
    return writeBasicRoute(currNode, destNode, EAST, 0);
  }

  if (yoff < 0) {
    return writeBasicRoute(currNode, destNode, NORTH, 0);
  }

  if (yoff > 0) {
    return writeBasicRoute(currNode, destNode, SOUTH, 0);
  }

  cerr << "ERROR: mesh routing found no route from " << currNode << " to " << destNode
       << " offset is " << xoff << ", " << yoff << endl;

  return true;
}
