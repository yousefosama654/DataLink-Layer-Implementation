#include "node.h"
#define GetCurrentDir _getcwd
Define_Module(Node);

void Node::initialize()
{
    // TODO - Generated method body
    windowStart = 0;
    windowEnd = par("WindowSize").intValue();
    messageIndex = 0;
    maxSeqNo = par("MaximumSequenceNumber").intValue() + 1;
    NACKs.resize(maxSeqNo, false);
    recivedMessages.resize(maxSeqNo, false);
    sentMessages.resize(maxSeqNo, false);
    delayFlag = false;
    lossFlag = false;
    isSender = false;
    delayID = -1;
    lossID = -1;
}
void Node::handleMessage(cMessage *msg)
{
    // double paramValue = par("DuplicationDelay");
    // EV << "hello from " << this->getName() << " value is " << paramValue << "\n";
    // EV << "hello from " << this->getName();

    Message_Base *mptr = check_and_cast<Message_Base *>(msg);

    // if the message is self message send the processed msg and start processing new msg
    if (msg->isSelfMessage())
    {
        // print here in the output file--> At time ..., Node[id]  [sent]  frame .....
        if (mptr->getType() == 1 || mptr->getType() == 0)
        {
            sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
            return;
        }
        if (lossFlag && lossID == mptr->getHeader())
        {
            lossFlag = false;
        }
        else if (delayFlag && delayID == mptr->getHeader())
        {
            delayFlag = false;
            sendDelayed(msg, par("ErrorDelay").doubleValue() + par("TransmissionDelay").doubleValue(), "out");
        }
        else
        {
            sendDelayed(msg, par("TransmissionDelay").doubleValue(), "out");
        }

        sendMessage("out");
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
            if (mptr->getType() == 1)
            {
                EV << "ACK" << endl;
            }
            else if (mptr->getType() == 0)
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
        // recieving ACK
        // ACK may be request for new frame
        // or ask for lost frame
        if (mptr->getType() == 1)
        {
            int Ack_no = mptr->getAck_no();
            // advance window

            for (int i = windowStart; i < messageIndex; i++)
            {
                if (i % maxSeqNo == Ack_no)
                {
                    if (sentMessages[Ack_no] == true && NACKs[Ack_no] == true)
                    {
                        Message_Base *new_msg = new Message_Base();
                        framing(new_msg, messages[i]);
                        new_msg->setHeader(i % maxSeqNo);
                        scheduleAt(simTime() + 0.001, new_msg);
                        EV << "resending frame " << i << endl;
                    }
                    else
                    {
                        {
                            int oldWindowStart = windowStart;
                            windowEnd += (i - windowStart);
                            windowStart += (i - windowStart);
                            for (int i = oldWindowStart; i < windowStart; i++)
                            {
                                sentMessages[i] = false;
                                NACKs[i] = false;
                            }
                        }
                    }
                }
            }
        }
        // recieving  NACK
        else if (mptr->getType() == 0)
        {
            int Ack_no = mptr->getAck_no();
            for (int i = windowStart; i < messageIndex; i++)
            {
                if (i % maxSeqNo == Ack_no)
                {
                    NACKs[Ack_no] = true;
                    Message_Base *new_msg = new Message_Base();
                    framing(new_msg, messages[i]);
                    new_msg->setHeader(i % maxSeqNo);
                    scheduleAt(simTime() + 0.001, new_msg);
                    EV << "resending frame " << i << endl;
                }
            }
        }
        // recievind data
        else
        {
            // TODO add de-framing--------------------------------------
            int seqNo = mptr->getHeader();
            // if data is already recived discared it
            if (recivedMessages[seqNo] == true)
                return;
            // check for error using crc
            // if(calculateCRC(mptr->getPayload()) != mptr->getTrailer())
            //     return;
            for (int i = windowStart; i < windowEnd; i++)
            {
                if (i % maxSeqNo == seqNo)
                {
                    // if data is in order send ACK then advance recieve window
                    if (i == windowStart)
                    {
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
                        int ack_no = (windowStart % maxSeqNo);
                        sendACK(ack_no, 1, "out");
                        break;
                    }
                    // if data is out of order send NACK
                    else
                    {
                        int type = 1;
                        if (NACKs[seqNo] == false)
                        {
                            type = (0);
                            NACKs[seqNo] = true;
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
    // mptr->setTrailer(parity);
    mptr->setTrailer(calculateCRC(modified));
    mptr->setType(2);
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

char Node::calculateCRC(std::string &payload)
{
    // CRC-8 polynomial: x^8 + x^2 + x + 1 (0x07)
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
