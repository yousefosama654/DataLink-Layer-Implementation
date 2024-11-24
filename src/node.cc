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

Define_Module(Node);

void Node::initialize()
{
    // TODO - Generated method body
}
void Node::handleMessage(cMessage *msg)
{
    EV << "hello from " << this->getName() << "\n";
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
void Node::modifyMessage(std::string &payload)
{
    int bitIdx = rand() % 8;
    int byteIdx = rand() % payload.length();
    payload[byteIdx] ^= (1 << bitIdx);
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