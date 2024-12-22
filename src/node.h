#ifndef __DATALINK_NODE_H_
#define __DATALINK_NODE_H_

#include <omnetpp.h>
#include "message_m.h"
#include <queue>
#include <map>
#include <fstream>
#include <iostream>
#include <bitset>
#include <string>
using namespace omnetpp;
using namespace std;
/**
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
  std::vector<bool> NACKs;
  std::vector<bool> ACKS;
  std::vector<bool> recivedMessages;
  std::vector<bool> sentMessages;
  std::vector<Message_Base *> timers;
  bool isSender;
  int messageIndex;
  int windowStart;
  int windowEnd;
  int maxSeqNo;
  bool delayFlag;
  bool lossFlag;
  bool duplicateFlag;
  int delayID;
  int lossID;
  int duplicateID;
  bool ackLossFlag;
  bool busy;
  std::queue<pair<int, bool>> messageQueue;
  int modifiedBit;
  map<int, string> toNetworkLayer;

  typedef enum
  {
    NACK = 0,
    ACK = 1,
    Data = 2,
    timeout = 3,
  } MsgType_t;

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  char calculateCRC(std::string payload);
  string stringStuffing(string payload);
  void framing(Message_Base *mptr, int seqNum, string payload, bool modifyFlag = false);
  void deframing(Message_Base *mptr);
  string modifyMessage(string payload);
  void openOutputFile();
  void fillOutputFile();
  void Timeout_print(int seqnum);
  void readLine_print(std::bitset<4> error);
  void transmitDataFrame_print(Message_Base *msg);
  void transmitColnlrolFrame_print(Message_Base *msg);
  void receivingDataFrame_print(int seqNo, string payload);
  std::string get_current_dir();
  void readInputFile(std::string &fileName, std::vector<std::bitset<4>> *errors, std::vector<std::string> *messages);
  void sendMessage(const char *gateName);
  void sendACK(int Ack_no, int type, const char *gateName);
  void scheduleTimeout(int seqNum);
  void transmitMessage(int seqNum, MsgType_t type, bool delay);
  virtual void finish() override;

public:
  // Output file for logs
  static std::fstream outputFile;
  static std::vector<std::string> outputBuffer;
};

std::fstream Node::outputFile = nullptr;
std::vector<std::string> Node::outputBuffer = {};
#endif
