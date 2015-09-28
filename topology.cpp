#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include <getopt.h>

using namespace std;

#define DIMS  2

int numNodes = 0;
int numSwitches = 0;
int nodesPerDim[DIMS] = { 0 };

int fattness = 1;

bool isTorus = false;
bool isAdaptive = false;
bool isCrossbar = false;

char * outFilename;
ofstream outFile;

bool processArguments ( int argc, char ** argv );
bool generateSwitches ( void );

bool generateMeshTopology ( int nodeId );
bool generateTorusTopology ( int nodeId );
bool generateCrossbarTopology ( int nodeId );

bool generateDeadlockFreeMeshRoute ( int currNode,
                                     int destNode );

bool generateDeadlockFreeTorusRoute ( int currNode,
                                      int destNode );

bool generateDeadlockFreeCrossbarRoute ( int currNode,
                                       int destNode );
typedef int XDirection;

enum Direction {
  NORTH,
  SOUTH,
  EAST,
  WEST,
  LOCAL
};


int main ( int argc, char ** argv ) {

  int i, j;
  std::string progname = argv[0];

  if ( processArguments ( argc, argv ) ) {
    cerr << "Usage: " << progname << " -n <numnodes> [-m|-t|-c] [-a] [-w <width>] <output filename>" << endl;
    cerr << "\t-t specifies a 2D torus" << endl;
    cerr << "\t-m specifies a 2D mesh" << endl;
    cerr << "\t-c specifies a crossbar" << endl;
    cerr << "\t-a specifies adaptive routing tables (not implemented)" << endl;
    cerr << "\t-w specifies a fat 2D torus/mesh with <width> nodes per router" << endl;
    return 1;
  }


  // Generate the boilerplate parameters
  outFile << "# Boilerplate stuff" << endl;
  if (isCrossbar)
	outFile << "ChannelLatency 1" << endl;
  else
	outFile << "ChannelLatency 3" << endl;
  outFile << "ChannelLatencyData 4" << endl;
  outFile << "ChannelLatencyControl 1" << endl;
  outFile << "LocalChannelLatencyDivider 1" << endl;
  outFile << "SwitchInputBuffers 6" << endl;
  outFile << "SwitchOutputBuffers 6" << endl;
  outFile << "SwitchInternalBuffersPerVC 6" << endl;
  outFile << endl;

  // Output all of the node->switch connections
  if ( generateSwitches() )
    return true;

  // Make topology
  if ( isTorus ) {
    outFile << endl << "# Topology for a " << numNodes << " node TORUS with " << fattness << " nodes per router" << endl;
  }
  else if(isCrossbar) {
    outFile << endl << "# Topology for a " << numNodes << " node crossbar with " << fattness << " nodes per router" << endl;
  }
  else {
    outFile << endl << "# Topology for a " << numNodes << " node MESH with " << fattness << " nodes per router" << endl;
  }

  for ( i = 0; i < numSwitches; i++ ) {
    if ( isTorus ) {
          if ( generateTorusTopology ( i ) ) return 1;
    }else if (isCrossbar){
          if ( generateCrossbarTopology ( i ) ) return 1;
    }else {
          if ( generateMeshTopology ( i ) ) return 1;
    }
  }

  outFile << endl << "# Deadlock-free routing tables" << endl;

  // For each switch, generate a routing table to each destination
  for ( i = 0; i < numSwitches; i++ ) {

    outFile << endl << "# Switch " << i << " -> *" << endl;

    // For each destination
    for ( j = 0; j < numNodes; j++ ) {
      if ( isTorus ) {
           if ( generateDeadlockFreeTorusRoute ( i, j ) ) return 1;
      }else if(isCrossbar){
           if ( generateDeadlockFreeCrossbarRoute ( i, j ) ) return 1;
      } else {
           if ( generateDeadlockFreeMeshRoute ( i, j ) )  return 1;
      }
    }

  }

  outFile.close();

  return 0;
}


bool processArguments ( int argc, char ** argv ) {

  static struct option long_options[] = {
    { "torus",		0,	nullptr,	't'},
    { "mesh",		0,	nullptr,	'm'},
    { "crossbar",       0,      nullptr,   'c'},
    { "adaptive",	0,	nullptr, 	'a'},
    { "width",		1,	nullptr,	'w'},
    { "nodes",		1,	nullptr,	'n'},
    { "file",		1,	nullptr,	'f'}
  };
  int c, index;
  while ( (c = getopt_long(argc, argv, "af:chmn:tw:", long_options, &index)) >= 0) {
    switch (c) {
      case 'a':
        isAdaptive = false;
        break;
      case 'f':
        outFilename = optarg;
        break;
      case 'c':
        isCrossbar = true;
        isTorus = false;
        break;
      case 'h':
        return true;
      case 'm':
        isTorus = false;
        isCrossbar = false;
        break;
      case 'n':
        numNodes = atoi(optarg);
        break;
      case 't':
        isTorus = true;
        isCrossbar = false;
        break;
      case 'w':
        fattness = atoi(optarg);
        break;
      case '?':
        cout << "Unrecognized option '" << optopt << "'" << endl;
        return true;
    }
  }


  if (optind < argc) {
    outFilename = argv[optind];
  }

  if(numNodes % fattness !=0) { 
    cerr << "ERROR: Numer of nodes must be divisible with the fattness!"<< endl;
    return true;
  }
  numSwitches = numNodes / fattness;


 int dim1= (int)sqrt ( (float)numSwitches );
 while((numSwitches%dim1) !=0) dim1--;
 
 nodesPerDim[1] = dim1;
 nodesPerDim[0] = numSwitches/dim1;
 

  for ( int i = 0; i < DIMS; i++ ) {
    cout << nodesPerDim[i] << " switches per dimension " << i << endl;
  }

  outFile.open ( outFilename );
  if ( !outFile.good() ) {
    cerr << "ERROR opening output file: " << outFilename << endl;
    return true;
  }

  return false;
}

bool generateSwitches ( void ) {
  int i;

  outFile << "# Basic Switch/Node connections" << endl;
  outFile << "NumNodes " << numNodes << endl;
  outFile << "NumSwitches " << numSwitches << endl;
  if(isCrossbar)  outFile << "SwitchPorts   " << (numSwitches + fattness) << endl;
  else outFile << "SwitchPorts   " << (4 + fattness) << endl;
  outFile << "SwitchBandwidth 4" << endl;
  outFile << endl;

  for ( i = 0; i < numNodes; i++ ) {
    outFile << "Top Node " << i << " -> Switch " << (i % numSwitches) << ":" << (int)(i / numSwitches) << endl;
  }

  return false;
}

int getXCoord ( int nodeId ) {
  return ( nodeId % nodesPerDim[0] );
}

int getYCoord ( int nodeId ) {
  return ( nodeId / nodesPerDim[0] );
}

int getNodeIdCoord ( int x, int y ) {
  int
  id;

  id = ( x + ( y * nodesPerDim[0] ) );

  if ( id >= numSwitches ) {
    cerr << "ERROR: node coordinates out of bounds: " << x << ", " << y << endl;
    exit ( 1 );
  }

  return id;
}

int getNodeIdOffset ( int nodeId, Direction dir ) {
  int
  x,
  y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );

  switch ( dir ) {
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
      exit ( 1 );
  }

  if ( isTorus ) {

    if ( x < 0 )
      x = nodesPerDim[0] - 1;

    if ( x >= nodesPerDim[0] )
      x = 0;

    if ( y < 0 )
      y = nodesPerDim[1] - 1;

    if ( y >= nodesPerDim[1] )
      y = 0;

  } else {

    // Invalid offset for a mesh
    if ( x < 0 || y < 0 ||
         x >= nodesPerDim[0] ||
         y >= nodesPerDim[1] ) {

      cerr << "ERROR: invalid offset for a mesh!" << endl;
      return -1;
    }
  }

  return getNodeIdCoord ( x, y );
}


ostream & operator <<  ( ostream & out, const Direction dir ) {

  switch ( dir ) {
    case NORTH:
      out << (fattness + 1);
      break;
    case SOUTH:
      out << (fattness + 3);
      break;
    case EAST:
      out << (fattness + 2);
      break;
    case WEST:
      out << (fattness + 0);
      break;
    case LOCAL:
      out << 0;
      break;
  };

  return out;
}



void printXDir(const XDirection dir, ostream& out)
{
   if(dir==numSwitches) out<<0;
   else out<<(fattness+dir);
}


bool writeS2SCrossbar (const int       node1,
                       const XDirection dir1,
                       const int       node2,
                       const XDirection dir2 ) {

  outFile << "Top Switch "
          << node1 << ":" ; printXDir(dir1,outFile); outFile << " -> Switch "
          << node2 << ":" ; printXDir(dir2,outFile); outFile << endl;

  return false;
}




bool writeS2S ( const int       node1,
                const Direction dir1,
                const int       node2,
                const Direction dir2 ) {

  outFile << "Top Switch "
          << node1 << ":" << dir1 << " -> Switch "
          << node2 << ":" << dir2 << endl;

  return false;
}

/* A switch looks like this:
 *  (port numbers on inside, port 0 to n are the local CPUs)
 *
 *                 N
 *                 |
 *        +-----------------+
 *        |       n+2       |
 *        |                 |
 *   W -- | n+1   0-n   n+3 | -- E
 *        |                 |
 *        |       n+4       |
 *        +-----------------+
 *                 |
 *                 S
 */
bool generateMeshTopology ( int nodeId ) {
  int x, y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );

  // East
  if ( x != nodesPerDim[0] - 1 )
    writeS2S ( getNodeIdCoord ( x, y ),
               EAST,
               getNodeIdCoord ( x + 1, y ),
               WEST );

  // South
  if ( y != nodesPerDim[1] - 1 )
    writeS2S ( getNodeIdCoord ( x, y ),
               SOUTH,
               getNodeIdCoord ( x, y + 1 ),
               NORTH );

  return false;
}

bool generateTorusTopology ( int nodeId ) {
  int
  x,
  y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );

  // East
  writeS2S ( getNodeIdCoord ( x, y ),
             EAST,
             getNodeIdCoord ( (x + 1) % nodesPerDim[0], y ),
             WEST );

  // South
  writeS2S ( getNodeIdCoord ( x, y ),
             SOUTH,
             getNodeIdCoord ( x, (y + 1) % nodesPerDim[1] ),
             NORTH );

  return false;
}


bool generateCrossbarTopology ( int nodeId ) {
  int
  x,
  y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );
  int src = getNodeIdCoord(x,y);
 
  for(int dest=nodeId+1; dest<numSwitches; dest++) {
     writeS2SCrossbar(src, dest, dest, src);          
  }
  
  return false;
}


bool writeBasicRoute ( int currNode,
                       int destNode,
                       int outPort,
                       int outVC ) {
  outFile << "Route Switch " << currNode << " -> " << destNode
          << " { " << outPort << ":" << outVC << " } " << endl;
  return false;
}

bool writeBasicRoute ( int currNode,
                       int destNode,
                       Direction outPort,
                       int outVC ) {
  outFile << "Route Switch " << currNode << " -> " << destNode
          << " { " << outPort << ":" << outVC << " } " << endl;
  return false;
}


bool writeBasicXRoute ( int currNode,
                       int destNode,
                       XDirection outPort,
                       int outVC ) {
  if(outVC == 0) {
    outFile << "Route Switch " << currNode << " -> " << destNode
           << " { ";  printXDir(outPort, outFile); outFile<< ":" << outVC << " } " << endl;
  }
  else {
    outFile << "Route Switch " << currNode << " -> " << destNode
           << " { "<<outVC<< ":" << 0 << " } " << endl;
 
  }
  return false;
}

#define ABS(X) ((X) > 0 ? (X) : -(X))

bool generateDeadlockFreeTorusRoute ( int currNode,
                                      int destNode ) {
  int
  xoff,
  yoff;

  // Trivial case, output to port 0, VC 0
  if ( currNode == (destNode % numSwitches) ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             (int)(destNode / numSwitches),
                             0 );
  }

  xoff =
    getXCoord ( (destNode % numSwitches) ) -
    getXCoord ( currNode );

  yoff =
    getYCoord ( (destNode % numSwitches) ) -
    getYCoord ( currNode );

  if ( xoff != 0 ) {

    if ( xoff > 0 && xoff < (nodesPerDim[0] / 2) ||
         xoff < (-nodesPerDim[0] / 2) ) {
      // Go EAST
      return writeBasicRoute ( currNode,
                               destNode,
                               EAST,
                               getXCoord ( destNode ) >
                               getXCoord ( currNode ) );
    } else {
      // Go WEST
      return writeBasicRoute ( currNode,
                               destNode,
                               WEST,
                               getXCoord ( destNode ) >
                               getXCoord ( currNode ) );
    }
  }

  if ( yoff > 0 && yoff < (nodesPerDim[1] / 2) ||
       yoff < (-nodesPerDim[1] / 2) ) {
    // Go SOUTH
    return writeBasicRoute ( currNode,
                             destNode,
                             SOUTH,
                             getYCoord ( destNode ) >
                             getYCoord ( currNode ) );
  } else {
    // Go NORTH
    return writeBasicRoute ( currNode,
                             destNode,
                             NORTH,
                             getYCoord ( destNode ) >
                             getYCoord ( currNode ) );
  }

  return false;
}


bool generateDeadlockFreeMeshRoute ( int currNode,
                                     int destNode ) {
  int xoff, yoff;

  xoff =
    getXCoord ( (destNode % numSwitches) ) -
    getXCoord ( currNode );

  yoff =
    getYCoord ( (destNode % numSwitches) ) -
    getYCoord ( currNode );

  // Trivial case, output to port 0, VC 0
  if ( currNode == (destNode % numSwitches) ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             (int)(destNode / numSwitches),
                             0 );
  }

  if ( xoff < 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             WEST,
                             0 );
  }

  if ( xoff > 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             EAST,
                             0 );
  }

  if ( yoff < 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             NORTH,
                             0 );
  }

  if ( yoff > 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             SOUTH,
                             0 );
  }

  cerr << "ERROR: mesh routing found no route from " << currNode << " to " << destNode << " offset is " << xoff << ", " << yoff <<  endl;

  return true;
}

bool generateDeadlockFreeCrossbarRoute ( int currNode,
                                     int destNode ) {
 
 int xs =  getXCoord ( currNode );
 int xd = getXCoord ( (destNode % numSwitches) );


 int ys =  getYCoord ( currNode );
 int yd =  getYCoord ( (destNode % numSwitches) );


  // Trivial case, output to port 0, VC 0
  if ( currNode == (destNode % numSwitches) ) {
    return writeBasicXRoute ( currNode,
                              destNode,
                              (int)(numSwitches),
                              destNode/numSwitches );
  }
  
  return writeBasicXRoute(currNode, destNode, (int)(destNode%numSwitches), 0);
 
  cerr << "ERROR: crossbar routing found no route from " << currNode << " to " << destNode <<  endl;

  return true;
}


