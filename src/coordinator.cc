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

#include "coordinator.h"
#include "message_m.h"
#include <iostream>
#include <fstream>
#define GetCurrentDir _getcwd
Define_Module(Coordinator);

void Coordinator::initialize()
{
    // TODO - Generated method body
    std::string fileName = "coordinator.txt";
    int nodeID;
    float startTime;
    readCoordinatorFile(fileName, nodeID, startTime);
    Message_Base *mmsg = new Message_Base();
    switch (nodeID)
    {
    case 0:
        mmsg->setPayload("input0.txt");
        sendDelayed(mmsg, startTime, "out0");
        break;
    case 1:
        mmsg->setPayload("input1.txt");
        sendDelayed(mmsg, startTime, "out1");
        break;
    }
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
void Coordinator::readCoordinatorFile(std::string &fileName, int &nodeID, float &start_t)
{
    // TODO - Generated method body
    // Open the file
    std::ifstream coordinator_file;
    // coordinator_file.open(fileName, std::ios::in);
    // coordinator_file.open(fileName);
    coordinator_file.open(get_current_dir() + "\\" + fileName, std::ios::in);
    // Return if file was not opened
    if (!coordinator_file.is_open())
    {
        std::cerr << "[COORDINATOR] Error opening file." << std::endl;
        return;
    }
    else
    {
        // Read the 2 numbers from file
        // NodeID is the first number, then start time in seconds
        coordinator_file >> nodeID;
        coordinator_file >> start_t;
        coordinator_file.close();
    }
}
std::string Coordinator::get_current_dir()
{
    char buff[FILENAME_MAX]; // create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}
