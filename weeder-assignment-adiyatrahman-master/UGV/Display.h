#pragma once

#include <NetworkedModule.h>
#include "Laser.h"
#include "GNSS.h"

#define DisplayPort 28000

ref class Display : public NetworkedModule {
public:
    //Default Contructor 
    Display();

    //Overloaded constructor
    Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps);

    // Establish TCP connection
    error_state connect(String^ hostName, int portNumber) override;

    // Communicate over TCP connection (includes any error checking required on data)
    error_state communicate() override;

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownThreads();

    // Get Shutdown signal for thread, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    //destructor
    ~Display();

    //Send data to the Matlab to the Matlab Display engine
    void sendDisplayData();

    void processHeartbeats();

private:
    //Inherited Variables
    //SM_ThreadManagement SM_TM_;
    // TcpClient^ Client;                   // Handle for TCP connection
    // NetworkStream^ Stream;               //Handle for TCP Stream Data
    // array<unsigned char>^ ReadData;      // Array to store sensor data
    // Add any additional data members or helper functions here
    array<unsigned char>^ SendData;      // Array to store sensor data
    Stopwatch^ Watch;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_GPS_;
};