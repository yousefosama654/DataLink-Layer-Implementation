#include "node.h"
#define GetCurrentDir _getcwd
Define_Module(Node);

void Node::initialize()
{
    // TODO:Get the parameters and put them in class variables
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
}
void Node::handleMessage(cMessage *msg)
{
    // double paramValue = par("DuplicationDelay");
    // EV << "hello from " << this->getName() << " value is " << paramValue << "\n";
    // EV << "hello from " << this->getName();

    Message_Base *mptr = check_and_cast<Message_Base *>(msg);

    // if the message is self message send the processed msg and start processing new msg
    // TODO: check the type of each self message and for each type call function sendDelayed()
    // TODO: in order not to get ACK/NACK before curr message is processing make a flag to check if the message is processing or not and buffer any ACKS or NACKS
    if (msg->isSelfMessage())
    {
        // TODO: call the timeout self message every time u send a message
        if (mptr->getType() == MsgType_t::timeout)
        {
            Timeout_print(mptr->getHeader());
            // TODO:resend the message
            EV << "here is time out " << mptr->getHeader() << endl;
            scheduleTimeout(mptr);
        }
        // Reciever send ACK or NACK
        // print here in the output file--> At time ..., Node[id]  [sent]  frame .....
        if (mptr->getType() == MsgType_t::ACK || mptr->getType() == MsgType_t::NACK)
        {
            EV << "finieshed PT ACK/NACK " << mptr->getAck_no() << endl;
            if (par("ACKLossProbability").doubleValue() < uniform(0, 1))
                sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
            return;
        }
        EV << "finieshed PT data " << mptr->getHeader() << endl;
        // Sender send the message
        if (lossFlag && lossID == mptr->getHeader())
        {
            lossFlag = false;
            sendMessage("out");
        }
        else if (delayFlag && delayID == mptr->getHeader())
        {
            delayFlag = false;
            sendDelayed(msg, par("ErrorDelay").doubleValue() + par("TransmissionDelay").doubleValue(), "out");
            scheduleTimeout(mptr);
            sendMessage("out");
        }
        else if (duplicateFlag && duplicateID == mptr->getHeader())
        {
            duplicateFlag = false;
            sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
            scheduleTimeout(mptr);
        }
        else
        {
            sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
            scheduleTimeout(mptr);
            sendMessage("out");
        }
        return;
    }
    // print the recived message from the other node
    else
    {
        if (mptr->getType() == 2)
        {
            EV << mptr->getHeader() << endl;
            EV << mptr->getPayload() << endl;
            EV << mptr->getTrailer() << endl;
        }
        else
        {
            if (mptr->getType() == MsgType_t::ACK)
            {
                EV << "ACK" << endl;
            }
            else if (mptr->getType() == MsgType_t::NACK)
            {
                EV << "NACK" << endl;
            }
            EV << mptr->getAck_no() << endl;
        }
    }
    // if I recieve message from coordinator read the input file
    if (std::string(mptr->getPayload()).rfind("input", 0) == 0)
    {
        std::string fileName = mptr->getPayload();
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
                // EV << "ACKED " << endl;
                // the ACKed frame is found
                if (i % maxSeqNo == Ack_no)
                {
                    // if the frame is already NACKed and ACKed again
                    if (NACKs[Ack_no] == true)
                    {
                        Message_Base *new_msg = new Message_Base();
                        framing(new_msg, messages[i]);
                        new_msg->setHeader(i % maxSeqNo);
                        EV << "simTime() + 0.001" << endl;
                        scheduleAt(simTime() + 0.001, new_msg);
                        EV << "resending frame " << i << endl;
                    }
                    else
                    {
                        // normal ack for some data
                        {
                            // old windowStart 5 windowEnd 10
                            int oldWindowStart = windowStart;
                            //  windowStart 5 WindowSize 4 5 6 7 8
                            windowStart = i;
                            windowEnd = windowStart + par("WindowSize").intValue();
                            for (int j = oldWindowStart; j < windowStart; j++)
                            {
                                sentMessages[j % maxSeqNo] = false;
                                NACKs[j % maxSeqNo] = false;
                                ACKS[j % maxSeqNo] = true;
                                // EV<<"ACKED "<<j<<endl;
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
            // EV << "entering sender NACK\n";
            for (int i = windowStart; i < messageIndex; i++)
            {
                // EV << "entering sender NACK loop " << i << endl;
                // EV << "windowStart " << windowStart << " messageIndex " << messageIndex << endl;
                if (i % maxSeqNo == Ack_no)
                {
                    NACKs[Ack_no] = true;
                    EV << "sender NACKS\n";
                    for (int i = 0; i < 8; i++)
                    {
                        EV << NACKs[i] << " ";
                    }
                    Message_Base *new_msg = new Message_Base();
                    framing(new_msg, messages[i]);
                    new_msg->setHeader(i % maxSeqNo);
                    scheduleAt(simTime() + 0.001, new_msg);
                    EV << "resending frame " << i << endl;
                }
            }
        }
        // recievind data (Reciever)
        else
        {
            // TODO: add de-framing
            int seqNo = mptr->getHeader();
            // if data is already recived discared it
            if (recivedMessages[seqNo] == true)
                return;
            // TODO:check for error using crc and if error stay silent
            // if(calculateCRC(mptr->getPayload()) != mptr->getTrailer())
            //     return;
            for (int i = windowStart; i < windowEnd; i++)
            {
                // i [1-5]
                // seqNo 2
                if (i % maxSeqNo == seqNo)
                {
                    // if data is in order send ACK then advance recieve window
                    if (i == windowStart)
                    {
                        EV << "in of order data " << seqNo << endl;
                        // recivedMessages
                        recivedMessages[seqNo] = true;
                        int i = seqNo;
                        while (recivedMessages[i] == true)
                        {
                            i++;
                            windowEnd += 1;
                            windowStart += 1;
                            recivedMessages[(windowEnd - 1) % maxSeqNo] = false;
                            NACKs[(windowEnd - 1) % maxSeqNo] = false;
                        }
                        EV << "printing recivedMessages \n";
                        EV << "windowStart " << windowStart << " windowEnd " << windowEnd << endl;
                        for (int i = 0; i < 8; i++)
                        {
                            EV << recivedMessages[i] << " ";
                        }
                        int ack_no = (windowStart % maxSeqNo);
                        // EV << "special case ack:: " << ack_no << " windowStart " << windowStart << " windowend " << windowEnd << endl;
                        sendACK(ack_no, 1, "out");
                        break;
                    }
                    // if data is out of order send NACK
                    else
                    {
                        EV << "out of order data " << seqNo << endl;
                        EV << " here is i " << i % maxSeqNo << endl;
                        recivedMessages[seqNo] = true;
                        int type = MsgType_t::ACK;
                        if (NACKs[windowStart % maxSeqNo] == false)
                        {
                            type = MsgType_t::NACK;
                            NACKs[windowStart % maxSeqNo] = true;
                            EV << "windowStart " << windowStart << " windowend " << windowEnd << endl;
                            for (int i = 0; i < 8; i++)
                            {
                                EV << NACKs[i] << " ";
                            }
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

void Node::framing(Message_Base *mptr, std::string &payload)
{
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

    mptr->setPayload(modified.c_str());
    // mptr->setHeader(seq);
    mptr->setTrailer(calculateCRC(modified));
    mptr->setType(2);
}
void Node::modifyMessage(Message_Base *msg)
{
    std::string payload = msg->getPayload();
    int bitIdx = rand() % 8;
    int byteIdx = rand() % payload.length();
    payload[byteIdx] += 1;
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
    // Return and print error if file was not opened
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
void Node::scheduleTimeout(Message_Base *mptr)
{
    int seqNum = mptr->getHeader();
    if (!timers[seqNum])
    {
        timers[seqNum] = new Message_Base("");
        timers[seqNum]->setHeader(mptr->getHeader());
        timers[seqNum]->setPayload(mptr->getPayload());
        timers[seqNum]->setTrailer(mptr->getTrailer());
        timers[seqNum]->setType(MsgType_t::timeout);
        timers[seqNum]->setAck_no(mptr->getAck_no());
        scheduleAfter(par("TimeoutInterval"), timers[seqNum]);
    }
}
void Node::sendMessage(const char *gateName)
{
    if (messageIndex >= windowEnd || messageIndex >= messages.size())
    {
        return;
    }
    // print here in the output file
    //     At time [.. starting processing time….. ],  Node[id] ,  Introducing channel error with  code
    // =[ …code in 4 bits… ] .
    Message_Base *new_msg = new Message_Base();
    new_msg->setHeader(messageIndex % maxSeqNo);
    framing(new_msg, messages[messageIndex]);
    std::bitset<4> error = errors[messageIndex];
    sentMessages[messageIndex % maxSeqNo] = true;
    messageIndex++;
    double delayTime = par("ProcessingDelay").doubleValue();
    if ((error & std::bitset<4>(errorType::Loss)) != std::bitset<4>(0))
    {
        lossFlag = true;
        lossID = new_msg->getHeader();
        scheduleAt(simTime() + delayTime, new_msg);
        return;
    }
    if ((error & std::bitset<4>(errorType::Modification)) != std::bitset<4>(0))
    {
        modifyMessage(new_msg);
    }
    if ((error & std::bitset<4>(errorType::Delay)) != std::bitset<4>(0))
    {
        delayID = new_msg->getHeader();
        delayFlag = true;
    }
    if ((error & std::bitset<4>(errorType::Duplication)) != std::bitset<4>(0))
    {
        // duplication
        duplicateFlag = true;
        duplicateID = new_msg->getHeader();
        scheduleAt(simTime() + delayTime, new_msg);
        Message_Base *duplicated_msg = new Message_Base(*new_msg);
        delayTime += par("DuplicationDelay").doubleValue();
        scheduleAt(simTime() + delayTime, duplicated_msg);
    }
    else
    {
        scheduleAt(simTime() + delayTime, new_msg);
    }
}

// TODO: implement the CRC function and check if this working
char Node::calculateCRC(std::string &payload)
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
    scheduleAt(simTime() + delayTime, new_msg);
}
