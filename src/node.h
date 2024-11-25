//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __DATALINK_NODE_H_
#define __DATALINK_NODE_H_

#include <omnetpp.h>
#include "message_m.h"
#include <fstream>
#include <iostream>
#include <bitset>
using namespace omnetpp;

/**
 * TODO - Generated class
 */
enum errorType
{
  Modification = 8,
  Loss = 4,
  Duplication = 2,
  Delay = 1
};

class Node : public cSimpleModule
{
private:
  std::vector<std::bitset<4>> errors;
  std::vector<std::string> messages;
  bool isSender;
  int messageIndex;
  int windowStart;
  int windowEnd;
  int maxSeqNo;

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  char calculateParity(std::string &payload);
  char calculateCRC(std::string &payload);
  void framing(Message_Base *mptr, std::string &payload);
  void modifyMessage(Message_Base *msg);
  void openOutputFile();
  void fillOutputFile();
  void Timeout_print(int seqnum);
  std::string get_current_dir();
  void readInputFile(std::string &fileName, std::vector<std::bitset<4>> *errors, std::vector<std::string> *messages);
  void sendMessage(Message_Base *msg, std::bitset<4> error, const char *gateName);

public:
  // Output file for logs
  std::fstream outputFile;
  std::vector<std::string> outputBuffer;
};
#endif
