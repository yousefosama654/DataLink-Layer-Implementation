#include "coordinator.h"
#include "message_m.h"
#include <iostream>
#include <fstream>
#define GetCurrentDir _getcwd
Define_Module(Coordinator);

void Coordinator::initialize()
{
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
}
void Coordinator::readCoordinatorFile(std::string &fileName, int &nodeID, float &start_t)
{
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
