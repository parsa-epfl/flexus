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

#define MAX_DIM 4

char *outFilename;
ofstream outFile;

int32_t nodesPerDim[MAX_DIM] = {0};
int32_t numDims = 0;
int32_t numNodes = 0;

bool processArguments(int32_t argc, char **argv);
bool generateSwitches(void);
bool generateTorusTopology(void);
bool generateRoutingTables(void);

int32_t main(int32_t argc, char **argv) {
  if (processArguments(argc, argv)) {
    cerr << "Usage: " << argv[0] << " out_file num_dimensions nodes_in_dim0 nodes_in_dim1 ..."
         << endl;
    return 1;
  }

  if (numDims != 3) {
    cout << " ********************* SUDDEN DEATH ****************************" << endl
         << " *** BRIAN WANTS THIS PROGRAM TO DIE IF YOU RUN IT IN SOMETHING " << endl
         << " *** OTHER THAN 3-D MODE BECAUSE IT IS NOT CONFIGURED FOR TRUSS " << endl
         << " *** EXCEPT FOR 4x4x2                                           " << endl;
    return 1;
  }

  // Generate the boilerplate parameters
  outFile << "# Boilerplate stuff" << endl;
  outFile << "ChannelLatency             100" << endl;
  outFile << "ChannelLatencyData         32" << endl;
  outFile << "ChannelLatencyControl      4" << endl;
  outFile << "LocalChannelLatencyDivider 8" << endl;
  outFile << "SwitchInputBuffers         5" << endl;
  outFile << "SwitchOutputBuffers        1" << endl;
  outFile << "SwitchInternalBuffersPerVC 1" << endl;
  outFile << endl;

  // Output all of the node->switch connections
  if (generateSwitches())
    return true;

  // Build the topology
  if (generateTorusTopology())
    return true;

  // Build the routing tables
  if (generateRoutingTables())
    return true;

  outFile.close();

  return 0;
}

bool processArguments(int32_t argc, char **argv) {
  int i = 1, dim = 0;

  if (argc < 3)
    return true;

  // Open output file
  outFilename = argv[i++];
  outFile.open(outFilename);
  if (!outFile.good()) {
    cerr << "ERROR opening output file: " << outFilename << endl;
    return true;
  }

  numDims = atoi(argv[i++]);
  if (numDims < 1 || numDims > MAX_DIM) {
    cerr << "ERROR: dimensions must be >= 1 and <= " << MAX_DIM << endl;
    return true;
  }

  if (i + numDims != argc) {
    cerr << "ERROR: node counts per dimension do not match dimension count" << endl;
    return true;
  }

  numNodes = 1;
  dim = 0;
  do {

    if (dim >= MAX_DIM) {
      cerr << "ERROR: too many dimensions listed" << endl;
      return true;
    }

    nodesPerDim[dim] = atoi(argv[i]);
    if (nodesPerDim[dim] <= 0) {
      cerr << "ERROR: nodes in dimension " << dim << " must be > 0" << endl;
      return true;
    }
    cout << " Dim " << dim << " has " << nodesPerDim[dim] << " nodes " << endl;

    numNodes = numNodes * nodesPerDim[dim];

    dim++;
    i++;
  } while (i < argc);

  return false;
}

bool generateSwitches(void) {
  int i;

  outFile << "# Basic Switch/Node connections for a " << numDims << " dimensional network " << endl;
  outFile << "NumNodes        " << numNodes << endl;
  outFile << "NumSwitches     " << numNodes << endl;
  outFile << "SwitchPorts     " << 2 * numDims + 1 << endl;
  outFile << "SwitchBandwidth " << 2 * numDims + 1 << endl;
  outFile << endl;

  for (i = 0; i < numNodes; i++) {
    outFile << "Top Node " << i << " -> Switch " << i << ":0" << endl;
  }

  return false;
}

bool getNodeCoord(const int32_t nodeId, int32_t *coord) {
  int32_t dim = 0, divisor = 1;

  for (dim = 0; dim < numDims; dim++) {
    coord[dim] = (nodeId / divisor) % nodesPerDim[dim];
    divisor = divisor * nodesPerDim[dim];
  }

  return false;
}

int32_t getNodeId(const int32_t *coord) {
  int32_t dim, nodeId = 0, multiplier = 1;

  for (dim = 0; dim < numDims; dim++) {
    nodeId += coord[dim] * multiplier;
    multiplier = multiplier * nodesPerDim[dim];
  }

  return nodeId;
}

bool writeS2S(const int32_t *coord1, const int32_t port1, const int32_t *coord2,
              const int32_t port2) {
  outFile << "Top Switch " << getNodeId(coord1) << ":" << port1 << " -> Switch "
          << getNodeId(coord2) << ":" << port2 << endl;

  return false;
}

int32_t getNonLocalPort(int32_t dim, bool posDir) {
  return (1 + (2 * dim) + (posDir ? 1 : 0));
}

bool generateTorusTopology(void) {
  int node = 0, dim = 0, i, port1, port2, coord1[MAX_DIM] = {0}, coord2[MAX_DIM] = {0};

  outFile << endl << "# Topology for a " << numNodes << " " << numDims << "-D torus" << endl;

  // For each node, connect it to its higher-numbered neighbors
  // in each dimension (wrap around at the edges).
  for (node = 0; node < numNodes; node++) {
    getNodeCoord(node, coord1);

    for (dim = 0; dim < numDims; dim++) {

      // Copy the coordinate and perturb by one in this dimension
      for (i = 0; i < numDims; i++)
        coord2[i] = coord1[i];
      coord2[dim] = (coord2[dim] + 1) % nodesPerDim[dim];

      port1 = getNonLocalPort(dim, true);
      port2 = getNonLocalPort(dim, false);
      writeS2S(coord1, port1, coord2, port2);
    }
  }

  return false;
}

bool writeBasicRoute(int32_t *coord1, int32_t *coord2, int32_t outPort, int32_t outVC) {
  outFile << "Route Switch " << getNodeId(coord1) << " -> " << getNodeId(coord2) << " { " << outPort
          << ":" << outVC << " } " << endl;
  return false;
}

bool generateRouteEntry(int32_t *coord1, int32_t *coord2) {
  int dim, offset;

  for (dim = 0; dim < numDims; dim++) {

    offset = coord2[dim] - coord1[dim];

    // Find the first dimension with different indicies
    if (offset == 0)
      continue;

    // These indicies are different, find the minimum path in this dimension
    if ((offset > 0 && offset < nodesPerDim[dim] / 2) || (offset < (-(nodesPerDim[dim] / 2)))) {

      return writeBasicRoute(coord1, coord2, getNonLocalPort(dim, true), offset > 0);
    } else {

      return writeBasicRoute(coord1, coord2, getNonLocalPort(dim, false), offset > 0);
    }
  }

  // We actually hit the destination.  Trivial route to local port
  return writeBasicRoute(coord1, coord2, 0, 0);
  return false;
}

bool generateRoutingTables(void) {
  int fromNode, toNode,

      fromCoord[MAX_DIM], toCoord[MAX_DIM];

  outFile << endl << "# Deadlock-free routing tables" << endl;

  for (fromNode = 0; fromNode < numNodes; fromNode++) {

    outFile << endl << "# Switch " << fromNode << " -> *" << endl;

    for (toNode = 0; toNode < numNodes; toNode++) {

      getNodeCoord(fromNode, fromCoord);
      getNodeCoord(toNode, toCoord);

      if (generateRouteEntry(fromCoord, toCoord))
        return true;
    }
  }

  return false;
}
