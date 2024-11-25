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

#include "node.h"
#define GetCurrentDir _getcwd
Define_Module(Node);

double delayTime = 0;

void Node::initialize()
{
    // TODO - Generated method body
    windowStart = 0;
    windowEnd = par("WindowSize").intValue();
    messageIndex = 0;
    maxSeqNo = par("WindowSize").intValue() * 2;

    EV << par("TransmissionDelay").doubleValue() << "\n";
    EV << par("ProcessingDelay").doubleValue() << "\n";
}
void Node::handleMessage(cMessage *msg)
{
    // double paramValue = par("DuplicationDelay");
    // EV << "hello from " << this->getName() << " value is " << paramValue << "\n";
    // EV << "hello from " << this->getName();

    // print the recived message
    EV << check_and_cast<Message_Base *>(msg)->getPayload() << endl;
    Message_Base *mptr = check_and_cast<Message_Base *>(msg);

    // if I recieve message from coordinator read the input file
    if (std::string(mptr->getPayload()).rfind("input", 0) == 0)
    {
        std::string fileName = mptr->getPayload();
        readInputFile(fileName, &errors, &messages);
        isSender = true;
    }

    if (isSender)
    {
        delayTime = 0;
        while (messageIndex < windowEnd && messageIndex < messages.size())
        {
            delayTime += par("ProcessingDelay").doubleValue();
            Message_Base *new_msg = new Message_Base();
            new_msg->setPayload(messages[messageIndex].c_str());
            new_msg->setType(0);
            new_msg->setHeader(messageIndex % maxSeqNo);
            new_msg->setTrailer(0);
            sendMessage(new_msg, errors[messageIndex], "out");
            messageIndex++;
        }
    }
}

char Node::calculateParity(std::string &payload)
{
    char parityByte = 0;
    int payloadSize = payload.size();
    for (int i = 0; i < payloadSize; ++i)
    {
        parityByte = (parityByte ^ payload[i]);
    }
    return parityByte;
}

void Node::framing(Message_Base *mptr, std::string &payload)
{
    // TODO - Generated method body
    std::string modified = "$";
    int payloadSize = payload.size();

    for (int i = 0; i < payloadSize; i++)
    {
        if (payload[i] == '$' || payload[i] == '/')
        {
            modified += "/";
        }
        modified += payload[i];
    }
    modified += "$";

    char parity = calculateParity(modified);

    mptr->setPayload(modified.c_str());
    // mptr->setHeader(seq);
    mptr->setTrailer(parity);
    mptr->setType(0);
}
void Node::modifyMessage(Message_Base *msg)
{
    std::string payload = msg->getPayload();
    int bitIdx = rand() % 8;
    int byteIdx = rand() % payload.length();
    payload[byteIdx] ^= (1 << bitIdx);
    msg->setPayload(payload.c_str());
}
void Node::openOutputFile()
{

    outputFile.open("output.txt", std::ios::out | std::ios::trunc);

    // Return if file was not opened
    if (!outputFile.is_open())
    {
        std::cerr << "[NODE] Error opening output file." << std::endl;
        return;
    }
}

void Node::fillOutputFile()
{
    openOutputFile();
    for (auto it : outputBuffer)
    {
        outputFile << it << std::endl;
    }
    outputFile.close();
}
void Node::Timeout_print(int seqnum)
{

    std::string line_to_print = "Time out event at time [" + simTime().str() + "], at Node [" + this->getName()[4] + "] for frame with seq_num=[" + std::to_string(seqnum) + "]; \n";
    std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}

std::string Node::get_current_dir()
{
    char buff[FILENAME_MAX]; // create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}
void Node::readInputFile(std::string &fileName, std::vector<std::bitset<4>> *errors, std::vector<std::string> *messages)
{
    // Open the file
    std::ifstream input_file;
    input_file.open(get_current_dir() + "\\" + fileName, std::ios::in);
    // Return if file was not opened
    if (!input_file.is_open())
    {
        std::cerr << "[NODE] Error opening file." << std::endl;
        return;
    }
    else
    {
        std::bitset<4> error;
        std::string message;
        while (input_file >> error)
        {
            std::getline(input_file, message);
            errors->push_back(error);
            messages->push_back(message);
        }
        input_file.close();
    }
}

void Node::sendMessage(Message_Base *msg, std::bitset<4> error, const char *gateName)
{

    if ((error & std::bitset<4>(errorType::Loss)) != std::bitset<4>(0))
    {
        return;
    }
    delayTime += par("TransmissionDelay").doubleValue();
    if ((error & std::bitset<4>(errorType::Modification)) != std::bitset<4>(0))
    {
        modifyMessage(msg);
    }
    if ((error & std::bitset<4>(errorType::Delay)) != std::bitset<4>(0))
    {
        delayTime += par("ErrorDelay").doubleValue();
    }
    if ((error & std::bitset<4>(errorType::Duplication)) != std::bitset<4>(0))
    {
        // duplication
        sendDelayed(msg, delayTime, gateName);
        Message_Base *duplicated_msg = new Message_Base(*msg);
        delayTime += par("DuplicationDelay").doubleValue();
        sendDelayed(duplicated_msg, delayTime, gateName);
    }
    else
    {
        sendDelayed(msg, delayTime, gateName);
    }
}
