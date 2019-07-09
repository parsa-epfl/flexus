// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#include "netcontainer.hpp"
#include <fstream>
#include <iostream>
#include <string.h>

namespace nNetShim {

// When do we validate the arguments and build the network?
enum ValidationOrder {
  VALIDATE_ANYTIME, // Token available before valiation
  VALIDATE_BEFORE,  // Token only available before validation
  VALIDATE_AFTER    // Token causes validation
};

struct TokenMap {
  const char *token;
  ValidationOrder order;
  bool (*func)(istream &infile, NetContainer *nc);
};

const TokenMap topTokens[] = {
    {"INCLUDE", VALIDATE_ANYTIME, NetContainer::handleInclude},
    {"CHANNELLATENCY", VALIDATE_BEFORE, NetContainer::handleChannelLatency},
    {"CHANNELLATENCYDATA", VALIDATE_BEFORE, NetContainer::handleChannelLatencyData},
    {"CHANNELLATENCYCONTROL", VALIDATE_BEFORE, NetContainer::handleChannelLatencyControl},
    {"CHANNELBANDWIDTH", VALIDATE_BEFORE, NetContainer::handleChannelBandwidth},
    {"LOCALCHANNELLATENCYDIVIDER", VALIDATE_BEFORE, NetContainer::handleLocalChannelLatencyDivider},
    {"NUMNODES", VALIDATE_BEFORE, NetContainer::handleNumNodes},
    {"NUMSWITCHES", VALIDATE_BEFORE, NetContainer::handleNumSwitches},
    {"SWITCHINPUTBUFFERS", VALIDATE_BEFORE, NetContainer::handleSwitchInputBuffers},
    {"SWITCHOUTPUTBUFFERS", VALIDATE_BEFORE, NetContainer::handleSwitchOutputBuffers},
    {"SWITCHINTERNALBUFFERSPERVC", VALIDATE_BEFORE, NetContainer::handleSwitchInternalBuffersPerVC},
    {"SWITCHBANDWIDTH", VALIDATE_BEFORE, NetContainer::handleSwitchBandwidth},
    {"SWITCHPORTS", VALIDATE_BEFORE, NetContainer::handleSwitchPorts},
    {"TOP", VALIDATE_AFTER, NetContainer::handleTopology},
    {"ROUTE", VALIDATE_AFTER, NetContainer::handleRoute}};

const char STR_NODE[] = "NODE";
const char STR_TO[] = "->";
const char STR_SWITCH[] = "SWITCH";
const char STR_OPENCURLY[] = "{";
const char STR_CLOSECURLY[] = "}";

#define TOP_TOKEN_COUNT (sizeof(topTokens) / sizeof(TokenMap))

NetContainer::NetContainer(void)
    : channels(nullptr), switches(nullptr), nodes(nullptr), mslHead(nullptr), activeMessages(0),

      channelLatency(-1), channelLatencyData(-1), channelLatencyControl(-1), channelBandwidth(-1),
      localChannelLatencyDivider(-1), numNodes(-1), numSwitches(-1), switchInputBuffers(-1),
      switchOutputBuffers(-1), switchInternalBuffersPerVC(-1), switchBandwidth(-1), switchPorts(-1),
      numChannels(0), maxChannelIndex(-1), openFiles(0) {
}

bool NetContainer::buildNetwork(const char *filename) {
  ifstream infile;

  infile.open(filename);

  if (!infile.good()) {
    std::cerr << "NetShim: error opening configuration/topology file: \"" << filename
              << "\".  Bailing out!" << endl;
    return true;
  }

  openFiles++;

  while (!infile.eof()) {
    if (readFirstToken(infile)) {
      std::cerr << " in file: " << filename << endl;
      infile.close();
      openFiles--;
      return true;
    }
  }

  infile.close();

  openFiles--;

  if (openFiles == 0) {

    // Validate the network
    if (validateNetwork()) {
      std::cerr << "Network validation failed" << endl;
      return true;
    }
  }

  return false;
}

bool NetContainer::validateParameters(void) {
  bool foundError = false;

#define CHECK_PARAM_GT(VAR, VAL)                                                                   \
  if (!((VAR) > VAL)) {                                                                            \
    std::cerr << "NetShim: parameter error: " #VAR " must be > " #VAL << endl;                     \
    foundError = true;                                                                             \
  }

  // We only do this once
  if (numChannels > 0)
    return false;

  CHECK_PARAM_GT(channelLatency, 0);
  CHECK_PARAM_GT(channelLatencyData, 0);
  CHECK_PARAM_GT(channelLatencyControl, 0);
  CHECK_PARAM_GT(localChannelLatencyDivider, 0);
  CHECK_PARAM_GT(numNodes, 0);
  CHECK_PARAM_GT(numSwitches, 0);
  CHECK_PARAM_GT(switchInputBuffers, 0);
  CHECK_PARAM_GT(switchOutputBuffers, 0);
  CHECK_PARAM_GT(switchInternalBuffersPerVC, -1);
  CHECK_PARAM_GT(switchBandwidth, 0);
  CHECK_PARAM_GT(switchPorts, 1);

#undef CHECK_PARAM_GT

  return foundError;
}

bool NetContainer::readStringToken(istream &infile, char *str) {
  // Skip whitespace
  ws(infile);

  if (infile.eof())
    return true;

  // Remove comments
  if (infile.peek() == '#') {
    infile.ignore(1024, '\n');
    return true;
  }

  // Read the string
  infile >> str;

  return false;
}

bool NetContainer::readIntToken(istream &infile, int32_t &i) {
  // Skip whitespace
  ws(infile);

  if (infile.eof())
    return true;

  if (infile.peek() == '#') {
    infile.ignore(1024, '\n');
    return true;
  }

  // Read the integer
  infile >> i;

  return false;
}

bool NetContainer::readTopologyDoubleToken(istream &infile, int32_t &tok1, int32_t &tok2,
                                           NetContainer *nc) {
  char c;

  const char *errMsg = "";

  if (readIntToken(infile, tok1)) {
    errMsg = "token part 1";
    goto error;
  }

  infile.get(c);
  if (c != ':')
    goto error;

  if (readIntToken(infile, tok2)) {
    errMsg = "token part 2";
    goto error;
  }

  return false;
error:
  std::cerr << "ERROR: Cannot parse topology 2-ple " << errMsg << endl;
  return true;
}

bool NetContainer::readTopologyTripleToken(istream &infile, int32_t &sw, int32_t &port, int32_t &vc,
                                           NetContainer *nc) {
  char c;

  const char *errMsg = "";

  if (readIntToken(infile, sw)) {
    errMsg = "Switch specifier";
    goto error;
  }

  infile.get(c);
  if (c != ':')
    goto error;

  if (readIntToken(infile, port)) {
    errMsg = "Port specifier";
    goto error;
  }

  infile.get(c);
  if (c != ':')
    goto error;

  if (readIntToken(infile, vc)) {
    errMsg = "VC specifier";
    goto error;
  }

  if (sw >= nc->numSwitches || sw < 0) {
    errMsg = "switch number of out range";
    goto error;
  }

  if (port >= nc->switchPorts || port < 0) {
    errMsg = "port number out of range";
    goto error;
  }

  if (vc > 1 || vc < 0) {
    errMsg = "VC number out of range";
    goto error;
  }

  return false;

error:
  std::cerr << "ERROR: Cannot parse topology 3-ple " << errMsg << endl;
  return true;
}

bool NetContainer::readFirstToken(istream &infile) {
  char firstTok[100];

  int i;

retry:

  ws(infile);

  if (infile.eof())
    return false;

  infile >> firstTok;

  // Handle comments by ignoring the rest of the line

  if (firstTok[0] == '#') {
    infile.ignore(1024, '\n');
    goto retry;
  }

  // Okay, we have a token, then.  Let's try to identify it.
  for (i = 0; i < (int)TOP_TOKEN_COUNT; i++) {
    if (strcasecmp(topTokens[i].token, firstTok) == 0) {

      // Recognized a token!

      // Check if we have to validate/build the network
      if (topTokens[i].order == VALIDATE_BEFORE && numChannels != 0) {
        std::cerr << "ERROR: token " << firstTok
                  << " must appear before topology and routing tokens!" << endl;
        return true;
      } else if (topTokens[i].order == VALIDATE_AFTER && numChannels == 0) {
        if (validateParameters()) {
          std::cerr << "ERROR: bad or missing parameters before "
                       "topology/routing section ";
          return true;
        }

        if (allocateNetworkStructures()) {
          std::cerr << "ERROR: allocating network structures." << endl;
          return true;
        }
      }

      // Call the handler function for this token
      if (topTokens[i].func(infile, this)) {
        std::cerr << "ERROR: parsing arguments: " << firstTok;
        return true;
      }
      return false;
    }
  }

  std::cerr << "ERROR: Cannot identify network simulator token: " << firstTok;

  return true;
}

bool NetContainer::drive(void) {
  int i;

#ifndef NS_STANDALONE
#ifdef NS_DEBUG
  currTime++;
#endif
#endif
  for (i = 0; i < numSwitches; i++) {
    if (switches[i]->drive())
      return true;
  }

  // Drive the input ports first
  for (i = 0; i < numSwitches; i++) {
    if (switches[i]->driveInputPorts())
      return true;
  }

  for (i = 0; i < numNodes; i++) {
    if (nodes[i]->driveInputPorts())
      return true;
  }

  for (i = 0; i < maxChannelIndex; i++) {
    if (channels[i]->drive())
      return true;
  }

  for (i = 0; i < numNodes; i++) {
    if (nodes[i]->drive())
      return true;
  }

  return false;
}

bool NetContainer::handleInclude(istream &infile, NetContainer *nc) {
  char filename[100];

  if (readStringToken(infile, filename))
    return true;

  // Continue with the included file at this point...
  return nc->buildNetwork(filename);
}

bool NetContainer::handleChannelLatency(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->channelLatency);
}

bool NetContainer::handleChannelLatencyData(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->channelLatencyData);
}

bool NetContainer::handleChannelLatencyControl(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->channelLatencyControl);
}

bool NetContainer::handleChannelBandwidth(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->channelBandwidth);
}

bool NetContainer::handleLocalChannelLatencyDivider(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->localChannelLatencyDivider);
}

bool NetContainer::handleNumNodes(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->numNodes);
}

bool NetContainer::handleNumSwitches(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->numSwitches);
}

bool NetContainer::handleSwitchInputBuffers(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->switchInputBuffers);
}

bool NetContainer::handleSwitchOutputBuffers(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->switchOutputBuffers);
}

bool NetContainer::handleSwitchInternalBuffersPerVC(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->switchInternalBuffersPerVC);
}

bool NetContainer::handleSwitchBandwidth(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->switchBandwidth);
}

bool NetContainer::handleSwitchPorts(istream &infile, NetContainer *nc) {
  return readIntToken(infile, nc->switchPorts);
}

bool NetContainer::readTopSrcDest(istream &infile, bool &isSwitch, int32_t &node, int32_t &sw,
                                  int32_t &port, NetContainer *nc) {
  char str[100];

  if (readStringToken(infile, str))
    return true;

  // Read in the first half of the topology specification
  //   Node x
  //   Switch x:y:z

  if (strcasecmp(STR_NODE, str) == 0) {
    if (readIntToken(infile, node))
      return true;

    isSwitch = false;

    if (node >= nc->numNodes) {
      std::cerr << "ERROR: topology node " << node << " is out of range" << endl;
      return true;
    }

  } else if (strcasecmp(STR_SWITCH, str) == 0) {

    if (readTopologyDoubleToken(infile, sw, port, nc))
      return true;

    isSwitch = true;

  } else {
    std::cerr << "ERROR: expected NODE or SWITCH when parsing: " << str;
    return true;
  }

  return false;
}

bool NetContainer::handleTopology(istream &infile, NetContainer *nc) {
  int

      node[2],
      sw[2], port[2];

  bool fromSwitch = false, toSwitch = false;

  char str[100];

  if (readTopSrcDest(infile, fromSwitch, node[0], sw[0], port[0], nc))
    return true;

  if (readStringToken(infile, str)) {
    return true;
  }

  if (strcasecmp(STR_TO, str)) {
    std::cerr << "ERROR: expected '" << STR_TO << "', not '" << str << "'" << endl;
    return true;
  }

  if (readTopSrcDest(infile, toSwitch, node[1], sw[1], port[1], nc))
    return true;

  if (!fromSwitch && !toSwitch) {
    std::cerr << "ERROR: topology declaration must have at least one switch endpoint" << endl;
    return true;
  }

  if (nc->maxChannelIndex >= (nc->numChannels - 1)) {
    std::cerr << "ERROR: too many channels specified!" << endl;
    return true;
  }

  // If we have a node, we'll always make it the source
  // and the switch the destination (order doesn't really matter in the
  // topology, but this makes the file format more flexible).
  //
  // Similarly for Node->SWITCH connections, we explicitly set the latency on
  // the switch side to be the local delay.
  if (!(fromSwitch && toSwitch)) {

    if (fromSwitch) {
      assert(!toSwitch);
      cerr << "Attaching node ";
      if (attachNodeChannels(nc, node[1]))
        return true;
      cerr << " to switch ";
      if (attachSwitchChannels(nc, sw[0], port[0], true))
        return true;
      cerr << endl;

      if (nc->switches[sw[0]]->setLocalDelayOnly(port[0]))
        return true;

    } else {
      assert(toSwitch);
      cerr << "Attaching node ";
      if (attachNodeChannels(nc, node[0]))
        return true;
      cerr << " to switch ";
      if (attachSwitchChannels(nc, sw[1], port[1], true))
        return true;
      cerr << endl;

      if (nc->switches[sw[1]]->setLocalDelayOnly(port[1]))
        return true;
    }

  } else {

    cerr << "Attaching switch ";
    if (attachSwitchChannels(nc, sw[0], port[0], false))
      return true;
    cerr << " to switch ";
    if (attachSwitchChannels(nc, sw[1], port[1], true))
      return true;
    cerr << endl;
  }

  nc->maxChannelIndex += 2;

  return false;
}

bool NetContainer::attachSwitchChannels(NetContainer *nc, const int32_t sw, const int32_t port,
                                        const bool destination) {
  const char *msg = "";

  if (nc->switches[sw]->attachChannel(nc->channels[nc->maxChannelIndex], port, destination)) {
    msg = "from";
    goto error;
  }

  if (nc->switches[sw]->attachChannel(nc->channels[nc->maxChannelIndex + 1], port, !destination)) {
    msg = "to";
    goto error;
  }

  std::cerr << sw << ":" << port;
  return false;

error:
  std::cerr << "ERROR: attaching " << msg << " switch " << sw << ":" << port << endl;
  return true;
}

bool NetContainer::attachNodeChannels(NetContainer *nc, const int32_t node) {
  if (nc->nodes[node]->addFromChannel(nc->channels[nc->maxChannelIndex]))
    goto error;

  if (nc->nodes[node]->addToChannel(nc->channels[nc->maxChannelIndex + 1]))
    goto error;

  nc->channels[nc->maxChannelIndex]->setLocalLatencyDivider(nc->localChannelLatencyDivider);
  nc->channels[nc->maxChannelIndex + 1]->setLocalLatencyDivider(nc->localChannelLatencyDivider);

  std::cerr << node;

  return false;

error:
  std::cerr << "ERROR: attaching node " << node << endl;
  return true;
}

bool NetContainer::handleRoute(istream &infile, NetContainer *nc) {
  char str[100];

  int sw, node, port, vc;

  // Routes look like this:
  // Route "Switch" sw "->" destnode "{" port:vc* "}"
  // where one or more port/outgoing vc pairs can be specified for adaptive
  // routing ( in order of priority for now )

  if (readStringToken(infile, str) || strcasecmp(STR_SWITCH, str)) {
    std::cerr << "ERROR: expected \'" << STR_SWITCH << "\' in route specification" << endl;
    return true;
  }

  if (readIntToken(infile, sw) || (sw >= nc->numSwitches || sw < 0)) {
    std::cerr << "ERROR: switch number " << sw << " in route specification is invalid" << endl;
    return true;
  }

  if (readStringToken(infile, str) || strcasecmp(STR_TO, str)) {
    std::cerr << "ERROR: expected \'" << STR_TO << "\' in route specification" << endl;
    return true;
  }

  if (readIntToken(infile, node) || (node >= nc->numNodes || node < 0)) {
    std::cerr << "ERROR: node number " << node << " in route specification is invalid" << endl;
    return true;
  }

  if (readStringToken(infile, str) || strcasecmp(STR_OPENCURLY, str)) {
    std::cerr << "ERROR: expected \'" << STR_OPENCURLY << "\' in route specification" << endl;
    return true;
  }

  // Now read in the list of destination ports for this packet
  ws(infile);
  while (infile.peek() != '}') {

    if (readTopologyDoubleToken(infile, port, vc, nc)) {
      std::cerr << "ERROR: unable to parse port:vc specification for route "
                   "from switch "
                << sw << " to node " << node << endl;
      return true;
    }

    if (port >= nc->switchPorts || port < 0) {
      std::cerr << "ERROR: port number " << port << " in route specification is invalid" << endl;
      return true;
    }

    if (vc >= MAX_NET_VC || vc < 0) {
      std::cerr << "ERROR: virtual channel number " << vc << " in route specification is invalid"
                << endl;
      return true;
    }

#if 0
    std::cout << " Adding routing table entry: sw " << sw << " -> " << node
              << " thru port " << port << endl;
#endif

    if (nc->switches[sw]->addRoutingEntry(node, port, vc)) {
      return true;
    }

    ws(infile);
  }

  if (readStringToken(infile, str) || strcasecmp(STR_CLOSECURLY, str)) {
    std::cerr << "ERROR: expected \'" << STR_CLOSECURLY << "\' in route specification" << endl;
    return true;
  }

  return false;
}

bool NetContainer::allocateNetworkStructures(void) {
  int i;

  if (numChannels > 0) {
    std::cerr << "ERROR: trying to allocate network twice." << endl;
    return true;
  }

  numChannels = (numSwitches * switchPorts) * 2;
  maxChannelIndex = 0;

  // Allocate channel array
  channels = new ChannelP[numChannels];
  for (i = 0; i < numChannels; i++)
    channels[i] = new Channel(i);

  // Allocate switch array
  switches = new NetSwitchP[numSwitches];
  for (i = 0; i < numSwitches; i++) {
    switches[i] = new NetSwitch(i, numNodes, switchPorts, switchInputBuffers, switchOutputBuffers,
                                switchInternalBuffersPerVC, switchBandwidth, channelLatency);
  }

  nodes = new NetNodeP[numNodes];
  for (i = 0; i < numNodes; i++) {
    nodes[i] = new NetNode(i, this);
  }

  return false;
}

bool NetContainer::validateNetwork(void) {
  int i;

  bool foundErrors = false;

  for (i = 0; i < numNodes; i++) {
    if (nodes[i]->checkTopology())
      foundErrors = true;
  }

  for (i = 0; i < numSwitches; i++) {
    if (switches[i]->checkTopology())
      foundErrors = true;

    if (switches[i]->checkRoutingTable())
      foundErrors = true;
  }

  return foundErrors;
}

bool NetContainer::insertMessage(MessageState *msg) {
  MessageStateList *newNode;

  if (msg->srcNode < 0 || msg->srcNode >= numNodes) {
    std::cerr << "Message has invalid source node: " << msg->srcNode << endl;
    return true;
  }

  if (msg->destNode < 0 || msg->destNode >= numNodes) {
    std::cerr << "Message has invalid destination node: " << msg->destNode << endl;
    return true;
  }

  if (msg->priority < 0 || msg->priority >= NUM_PRIORITIES) {
    std::cerr << "Message has an invalid priority: " << msg->priority << endl;
    return true;
  }

  newNode = allocMessageStateList(msg);

  if (mslHead == nullptr) {
    mslHead = mslTail = newNode;
  } else {
    mslTail->next = newNode;
    newNode->prev = mslTail;
    mslTail = newNode;
  }

  msg->myList = newNode;

  // TransmitLatency is taken from the size field of a flexus network transport
  // We translate this into a data or control latency here.
  if (msg->flexusInFastMode) {
    msg->transmitLatency = 1;
  } else if (channelBandwidth > 0) {
    // Use fancy math to always round up
    msg->transmitLatency = (msg->transmitLatency + channelBandwidth - 1) / channelBandwidth;
  } else {
    if (msg->transmitLatency > 8) {
      msg->transmitLatency = channelLatencyData;
    } else {
      msg->transmitLatency = channelLatencyControl;
    }
  }

  msg->networkVC = BUILD_VC(msg->priority, 0);

  activeMessages++;

  if (nodes[msg->srcNode]->insertMessage(msg))
    return true;

  return false;
}

bool NetContainer::deliverMessage(MessageState *msg) {
  assert(msg->myList != nullptr);

  // Deliver the message to the flexus node
  if (deliverMessagePtr(msg)) {
    return true;
  }

  if (msg->myList == mslHead) {
    mslHead = mslHead->next;
  } else {
    msg->myList->prev->next = msg->myList->next;
  }

  if (msg->myList == mslTail) {
    mslTail = mslTail->prev;
  } else {
    msg->myList->next->prev = msg->myList->prev;
  }

  freeMessageStateList(msg->myList);

  activeMessages--;

  if (activeMessages == 0) {
    assert(mslHead == nullptr && mslTail == nullptr);
  }

  // Clean up the message
  freeMessageState(msg);

  return false;
}

bool NetContainer::dumpState(ostream &out, const int32_t flags) {
  int i;

  out << "Network state dump" << endl;
  out << "------------------" << endl << endl;

  if (flags & NS_DUMP_METAINFO) {
    out << "Active messages: " << getActiveMessages() << endl;
    if (mslHead != nullptr) {
      out << "Oldest message serial: " << mslHead->msg->serial << endl;
      out << "Newest message serial: " << mslTail->msg->serial << endl;
    }
    out << endl;
  }

  if (flags & NS_DUMP_NODES) {
    for (i = 0; i < numNodes; i++) {
      if (nodes[i]->dumpState(out))
        return true;
    }
  }

  if (flags & NS_DUMP_CHANNELS) {
    for (i = 0; i < maxChannelIndex; i++) {
      if (channels[i]->dumpState(out))
        return true;
    }
  }

  if (flags & NS_DUMP_SWITCHES) {
    for (i = 0; i < numSwitches; i++) {
      if (switches[i]->dumpState(out, flags & NS_DUMP_SWITCHES))
        return true;
    }
  }

  return false;
}

} // namespace nNetShim
