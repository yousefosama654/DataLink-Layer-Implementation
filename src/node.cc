#include "node.h"
#define GetCurrentDir _getcwd
Define_Module(Node);

void Node::initialize()
{
    // TODO - Generated method body
    delayTime = 0;
    windowStart = 0;
    windowEnd = par("WindowSize").intValue();
    messageIndex = 0;
    maxSeqNo = par("WindowSize").intValue() * 2;
}
void Node::handleMessage(cMessage *msg)
{
    // double paramValue = par("DuplicationDelay");
    // EV << "hello from " << this->getName() << " value is " << paramValue << "\n";
    // EV << "hello from " << this->getName();

    Message_Base *mptr = check_and_cast<Message_Base *>(msg);

    // print the recived message
    if (mptr->getType() == 0)
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
        else if (mptr->getType() == 2)
        {
            EV << "NACK" << endl;
        }
        EV << mptr->getAck_no() << endl;
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
        if (mptr->getType() == 1)
        {
            int Ack_no = mptr->getAck_no();
            // advance window
            for (int i = windowStart; i < messageIndex; i++)
            {
                if (i % maxSeqNo == Ack_no)
                {
                    windowEnd += (i - windowStart);
                    windowStart += (i - windowStart);
                }
            }
        }
        // recieving  NACK
        else if (mptr->getType() == 2)
        {
            int Ack_no = mptr->getAck_no();
            for (int i = windowStart; i < messageIndex; i++)
            {
                if (i % maxSeqNo == Ack_no)
                {
                    Message_Base *new_msg = new Message_Base();
                    new_msg->setPayload(messages[i].c_str());
                    new_msg->setType(0);
                    new_msg->setHeader(i % maxSeqNo);
                    new_msg->setTrailer(0);
                    sendMessage(new_msg, std::bitset<4>(0), "out");
                }
            }
        }
        // recievind data
        else
        {
            // assume data is true --> check for error using crc
            // assume out of order data discarded --> add bool vector for messages to mark the recived data out of order
            // add de-framing
            int seqNo = mptr->getHeader();
            // advance window
            for (int i = windowStart; i < windowEnd; i++)
            {
                if (i % maxSeqNo == seqNo)
                {
                    if (i == windowStart)
                    {
                        Message_Base *new_msg = new Message_Base();
                        new_msg->setType(1);
                        new_msg->setAck_no((mptr->getHeader() + 1) % maxSeqNo);
                        sendMessage(new_msg, std::bitset<4>(0), "out");
                        windowEnd += 1;
                        windowStart += 1;
                        break;
                    }
                    else
                    {
                        Message_Base *new_msg = new Message_Base();
                        new_msg->setType(2);
                        new_msg->setAck_no((windowStart) % maxSeqNo);
                        sendMessage(new_msg, std::bitset<4>(0), "out");
                        break;
                    }
                }
            }
        }
    }

    if (isSender)
    {
        while (messageIndex < windowEnd && messageIndex < messages.size())
        {
            delayTime += par("ProcessingDelay").doubleValue();
            Message_Base *new_msg = new Message_Base();
            // new_msg->setPayload(messages[messageIndex].c_str());
            // add framing
            // new_msg->setType(0);
            new_msg->setHeader(messageIndex % maxSeqNo);
            // new_msg->setTrailer(0);
            framing(new_msg, messages[messageIndex]);
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
    // mptr->setTrailer(parity);
    mptr->setTrailer(calculateCRC(modified));
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
