#include "netcontainer.hpp"
#include "regress.hpp"

using namespace nNetShim;
using namespace std;

NetContainer
* nc = nullptr;

int32_t main ( int32_t argc, char ** argv ) {
  if ( argc != 2 ) {
    cerr << "Usage: " << argv[0] << " config_file" << endl;
    return 1;
  }

  cout << "Creating NetContainer." << endl;
  nc = new NetContainer();

  cout << "Building network from " << argv[1] << endl;

  if ( nc->buildNetwork ( argv[1] ) ) {
    cout << "Error building network." << endl;
    return 1;
  }

  cout << "Network built successfully." << endl;

  runRegressSuite();

  return 0;
}

