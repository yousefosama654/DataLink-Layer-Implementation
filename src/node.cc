#include "node.h"
#define GetCurrentDir _getcwd
Define_Module(Node);

void Node::initialize()
{
    windowStart = 0;
    windowEnd = par("WindowSize").intValue();
    messageIndex = 0;
    maxSeqNo = par("MaximumSequenceNumber").intValue() + 1;
    NACKs.resize(maxSeqNo, false);
    recivedMessages.resize(maxSeqNo, false);
    sentMessages.resize(maxSeqNo, false);
    timers.resize(maxSeqNo, nullptr);
    ACKS.resize(maxSeqNo, false);
    delayFlag = false;
    lossFlag = false;
    duplicateFlag = false;
    isSender = false;
    delayID = -1;
    lossID = -1;
    duplicateID = -1;
    ackLossFlag = false;
    busy = false;
}
void Node::transmitMessage(int seqNum, MsgType_t type)
{
    busy = true;
    for (int i = windowStart; i < windowEnd; i++)
    {
        if (i % maxSeqNo == seqNum)
        {
            Message_Base *new_msg = new Message_Base();
            framing(new_msg, seqNum, messages[i]);
            scheduleAfter(par("ProcessingDelay").doubleValue()+0.001, new_msg);
        }
    }
}
void Node::handleMessage(cMessage *msg)
{
    Message_Base *mptr = check_and_cast<Message_Base *>(msg);
    // if the message is self message send the processed msg and start processing new msg
    if (msg->isSelfMessage())
    {
        if (mptr->getType() == MsgType_t::timeout)
        {
            Timeout_print(mptr->getHeader());
            // resend the message
            
            if (busy == false)
                transmitMessage(mptr->getHeader(), MsgType_t::Data);
            else
            {
                messageQueue.push(std::make_pair(mptr->getHeader(), false));
                
            }
            return;
        }
        // Reciever send ACK or NACK
        // print here in the output file--> At time ..., Node[id]  [sent]  frame .....
        if (mptr->getType() == MsgType_t::ACK || mptr->getType() == MsgType_t::NACK)
        {
            if (par("ACKLossProbability").doubleValue() < uniform(0, 1))
                sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
            else
                ackLossFlag = true;

            transmitColnlrolFrame_print(mptr);
            return;
        }
        double totalDelayTime = par("TransmissionDelay").doubleValue();
        // Sender send the message

        if (lossFlag)
        {
            lossFlag = false;
            busy = false;

            sendMessage("out");
            scheduleTimeout(mptr->getHeader());
            return;
        }
        if (delayFlag)
        {
            if (duplicateFlag == false)
            {
                delayFlag = false;
            }
            totalDelayTime += par("ErrorDelay").doubleValue();
        }
        transmitDataFrame_print(mptr);
        sendDelayed(mptr, totalDelayTime, "out");
        scheduleTimeout(mptr->getHeader());
        if (duplicateFlag)
        {
            // in order nto to read the next line directly
            duplicateFlag = false;
            busy = true;
        }
        else
        {
            busy = false;
            sendMessage("out");
        }
        return;
    }
    
    // if I recieve message from coordinator read the input file
    if (string(mptr->getPayload()).rfind("input", 0) == 0)
    {
        string fileName = mptr->getPayload();
        readInputFile(fileName, &errors, &messages);
        isSender = true;
    }
    else
    {
        // recieving ACK (Sender)
        // ACK may be request for new frame
        // or ask for lost frame
        if (mptr->getType() == MsgType_t::ACK)
        {
            int Ack_no = mptr->getAck_no();
            // Searching for which frame is ACKed
            for (int i = windowStart; i <= messageIndex; i++)
            {
                // the ACKed frame is found
                if (i % maxSeqNo == Ack_no)
                {
                    // if the frame is already NACKed and ACKed again
                    if (NACKs[Ack_no] == true)
                    {
                        if (busy == false)
                        {
                            Message_Base *new_msg = new Message_Base();
                            framing(new_msg, i % maxSeqNo, messages[i]);
                            scheduleAfter(par("ProcessingDelay").doubleValue()+0.001, new_msg);
                        }
                        else
                        {
                            messageQueue.push(std::make_pair(Ack_no, true));
                        }
                    }
                    else
                    {
                        // normal ack for some data
                        {
                            int oldWindowStart = windowStart;
                            windowStart = i;
                            windowEnd = windowStart + par("WindowSize").intValue();
                            for (int j = oldWindowStart; j < windowStart; j++)
                            {
                                sentMessages[j % maxSeqNo] = false;
                                NACKs[j % maxSeqNo] = false;
                                ACKS[j % maxSeqNo] = true;
                                cancelAndDelete(timers[j % maxSeqNo]);
                                timers[j % maxSeqNo] = nullptr;
                            }
                        }
                    }
                    break;
                }
            }
        }
        // recieving  NACK (Sender)
        else if (mptr->getType() == 0)
        {
            int Ack_no = mptr->getAck_no();
            for (int i = windowStart; i < messageIndex; i++)
            {

                if (i % maxSeqNo == Ack_no)
                {
                    NACKs[Ack_no] = true;
                    
                    if (busy == false)
                    {
                        Message_Base *new_msg = new Message_Base();
                        framing(new_msg, i % maxSeqNo, messages[i]);
                        scheduleAfter(par("ProcessingDelay").doubleValue()+0.001, new_msg);
                    }
                    else
                    {
                        messageQueue.push(std::make_pair(Ack_no, true));
                    }
                }
            }
        }
        // recievind data (Reciever)
        else
        {
            int seqNo = mptr->getHeader();
            // if data is already recived discared it
            if (recivedMessages[seqNo] == true)
                return;
            if (calculateCRC(string(mptr->getPayload())) != mptr->getTrailer())
                return;
            for (int i = windowStart; i < windowEnd; i++)
            {
                if (i % maxSeqNo == seqNo)
                {
                    // if data is in order send ACK then advance recieve window
                    if (i == windowStart)
                    {

                        // if the data in order so write it in the output file and send to the network Layer
                        deframing(mptr);
                        receivingDataFrame_print(mptr->getHeader(), std::string(mptr->getPayload()));
                        int turnNo = (seqNo + 1) % maxSeqNo;
                        auto it = toNetworkLayer.find(turnNo);
                        while (it != toNetworkLayer.end())
                        {
                            receivingDataFrame_print(turnNo, it->second);
                            turnNo = (turnNo + 1) % maxSeqNo;
                            it = toNetworkLayer.find(turnNo);
                        }
                        // recivedMessages
                        recivedMessages[seqNo] = true;
                        int j = seqNo;
                        while (recivedMessages[j % maxSeqNo] == true)
                        {
                            j++;
                            windowEnd += 1;
                            windowStart += 1;
                            recivedMessages[(windowEnd - 1) % maxSeqNo] = false;
                            NACKs[(windowEnd - 1) % maxSeqNo] = false;
                        }
                        
                        int ack_no = (windowStart % maxSeqNo);
                        sendACK(ack_no, 1, "out");
                        break;
                    }
                    // if data is out of order send NACK
                    else
                    {
                        // insert to be printed in order when we send it to Network Layer
                        deframing(mptr);
                        toNetworkLayer.insert({seqNo, std::string(mptr->getPayload())});
                        recivedMessages[seqNo] = true;
                        int type = MsgType_t::ACK;
                        if (NACKs[windowStart % maxSeqNo] == false)
                        {
                            type = MsgType_t::NACK;
                            NACKs[windowStart % maxSeqNo] = true;
                        }
                        int ack_no = ((windowStart) % maxSeqNo);
                        sendACK(ack_no, type, "out");
                        break;
                    }
                }
            }
        }
    }
    if (isSender)
    {
        sendMessage("out");
    }
}

string Node::stringStuffing(string payload)
{
    string stuffed = "$";
    int payloadSize = payload.size();
    for (int i = 0; i < payloadSize; i++)
    {
        if (payload[i] == '$' || payload[i] == '/')
        {
            stuffed += "/";
        }
        stuffed += payload[i];
    }
    stuffed += "$";
    return stuffed;
}
void Node::framing(Message_Base *mptr, int seqNum, string payload, bool modifyFlag)
{
    string modifiedPayload = modifyMessage(payload);
    string stuffedPayload = stringStuffing(payload);
    string stuffedmodifiedPayload = stringStuffing(modifiedPayload);
    mptr->setPayload((modifyFlag) ? stuffedmodifiedPayload.c_str() : stuffedPayload.c_str());
    mptr->setHeader(seqNum);
    mptr->setTrailer(calculateCRC(stuffedPayload));
    mptr->setType(MsgType_t::Data);
}

void Node::deframing(Message_Base *mptr)
{
    string stuffed = mptr->getPayload();
    // Remove the start and end flag
    stuffed = stuffed.substr(1, stuffed.size() - 2);
    string unstuffed;
    int stuffedSize = stuffed.size();
    for (int i = 0; i < stuffedSize; i++)
    {
        if (stuffed[i] == '/')
        {
            // Skip the escape character and take the next character as is
            i++;
        }
        unstuffed += stuffed[i];
    }
    mptr->setPayload(unstuffed.c_str());
}

string Node::modifyMessage(string payload)
{
    int byteIdx = rand() % payload.length();
    payload[byteIdx] += 1;
    modifiedBit = (byteIdx + 1) * 8;
    return payload.c_str();
}
void Node::openOutputFile()
{
    outputFile.open("output.txt", ios::out | ios::trunc);
    // Return if file was not opened
    if (!outputFile.is_open())
    {
        cerr << "[NODE] Error opening output file." << endl;
        return;
    }
}

string Node::get_current_dir()
{
    char buff[FILENAME_MAX]; // create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    string current_working_dir(buff);
    return current_working_dir;
}
void Node::readInputFile(string &fileName, vector<bitset<4>> *errors, vector<string> *messages)
{
    // Open the file
    ifstream input_file;
    input_file.open(get_current_dir() + "\\" + fileName, ios::in);
    // Return and print error if file was not opened
    if (!input_file.is_open())
    {
        cerr << "[NODE] Error opening file." << endl;
        return;
    }
    else
    {
        bitset<4> error;
        string message;
        while (input_file >> error)
        {
            getline(input_file, message);
            errors->push_back(error);
            if (!message.empty() && message[0] == ' ')
            {
                message.erase(0, 1);
            }
            messages->push_back(message);
        }
        input_file.close();
    }
}
void Node::scheduleTimeout(int seqNum)
{
    // int seqNum = mptr->getHeader();
    if (!timers[seqNum])
    {
        timers[seqNum] = new Message_Base("");
        timers[seqNum]->setHeader(seqNum);
        timers[seqNum]->setType(MsgType_t::timeout);
        scheduleAfter(par("TimeoutInterval"), timers[seqNum]);
    }
}
void Node::sendMessage(const char *gateName)
{
    // i am processing something right now so i can't send any message
    if (busy == true)
        return;
    if (!messageQueue.empty())
    {
        int seqNum = messageQueue.front().first;
        transmitMessage(seqNum, MsgType_t::Data);
        messageQueue.pop();
        return;
    }
    if (messageIndex >= windowEnd || messageIndex >= messages.size())
    {
        return;
    }
    busy = true;
    delayFlag = false;
    lossFlag = false;
    duplicateFlag = false;


    Message_Base *new_msg = new Message_Base();
    bitset<4> error = errors[messageIndex];
    sentMessages[messageIndex % maxSeqNo] = true;
    framing(new_msg, messageIndex % maxSeqNo, messages[messageIndex], error[3]);
    readLine_print(error);
    messageIndex++;
    if ((error & bitset<4>(errorType::Loss)) != bitset<4>(0))
    {
        lossFlag = true;
        scheduleAfter(par("ProcessingDelay"), new_msg);
        return;
    }
    if ((error & bitset<4>(errorType::Delay)) != bitset<4>(0))
    {
        delayFlag = true;
    }
    if ((error & bitset<4>(errorType::Duplication)) != bitset<4>(0))
    {
        // duplication
        duplicateFlag = true;
        Message_Base *duplicated_msg = new Message_Base(*new_msg);
        scheduleAfter(par("DuplicationDelay").doubleValue() + par("ProcessingDelay").doubleValue(), duplicated_msg);
    }

    scheduleAfter(par("ProcessingDelay"), new_msg);
}

char Node::calculateCRC(string payload)
{
    // CRC-8 polynomial: x^2 + x + 1 (0x07)
    uint8_t polynomial = par("CRCpolynomial").intValue();
    uint8_t crc = 0x00;
    for (char &byte : payload)
    {
        crc ^= static_cast<uint8_t>(byte);
        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ polynomial;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return static_cast<char>(crc);
}

void Node::sendACK(int Ack_no, int type, const char *gateName)
{
    double delayTime = par("ProcessingDelay").doubleValue();
    Message_Base *new_msg = new Message_Base();
    new_msg->setType(type);
    new_msg->setAck_no(Ack_no);
    scheduleAfter(delayTime, new_msg);
}

void Node::finish()
{
    fillOutputFile();
}

void Node::fillOutputFile()
{
    openOutputFile();
    for (auto it : outputBuffer)
    {
        outputFile << it;
    }
    outputFile.close();
}
void Node::Timeout_print(int seqnum)
{

    std::string line_to_print = "Time out event at time [" + simTime().str() + "], at Node [" + this->getName()[4] + "] for frame with seq_num=[" + std::to_string(seqnum) + "]; \n";
    // std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}

void Node::readLine_print(std::bitset<4> error)
{
    std::string line_to_print = "At time [" + simTime().str() + "], at Node [" + this->getName()[4] + "] Introducing channel error with  code [" + error.to_string() + "]; \n";
    // std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}

void Node::transmitDataFrame_print(Message_Base *msg)
{
    int num = messageIndex - 1;
    std::bitset<4> error = errors[num];
    // Correct the delayValue computation
    double delayValue = (error[0] == 1) ? par("ErrorDelay").doubleValue() : 0;
    int dupValue = (error[1] == 0) ? 0 : (duplicateFlag == true) ? 1
                                                                 : 2;
    int modValue = (error[3] == 0) ? -1 : modifiedBit;
    // Correct the multi-line string concatenation
    std::string line_to_print =
        "at time [" + simTime().str() +
        "], at Node [" + this->getName()[4] +
        "]  frame with seq_num = [" + std::to_string(msg->getHeader()) +
        "] and payload = [" + msg->getPayload() +
        "] and trailer = [" + msg->getTrailer() +
        "] Modified [" + std::to_string(modValue) +
        "], Lost [" + std::to_string(error[2]) +
        "] Duplicate[" + std::to_string(dupValue) +
        "], Delay [" + std::to_string(delayValue) + "]  \n";
    // std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}

void Node::transmitColnlrolFrame_print(Message_Base *msg)
{

    std::string type = (msg->getType() == MsgType_t::ACK) ? "ACK" : "NACK";
    std::string lossValue = (ackLossFlag) ? "yes" : "No";
    ackLossFlag = false;
    // Correct the multi-line string concatenation
    std::string line_to_print =
        "at time [" + simTime().str() +
        "], at Node [" + this->getName()[4] +
        "]  Sending [" + type +
        "] with number = [" + std::to_string(msg->getAck_no()) +
        "] loss = [" + lossValue + "]  \n";
    // std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}

void Node::receivingDataFrame_print(int seqNo, string payload)
{
    // Uploading payload=[…..] and seq_num =[…]  to the network layer
    std::string line_to_print =
        "at time [" + simTime().str() +
        "] Uploading payload [" + std::string(payload) +
        "] and seq_num = [" + std::to_string(seqNo) +
        "] to the network layer  \n";
    // std::cout << line_to_print << std::endl;
    outputBuffer.push_back(line_to_print);
}
