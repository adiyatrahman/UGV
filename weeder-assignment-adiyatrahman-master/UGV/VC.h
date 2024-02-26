#pragma once

#include <NetworkedModule.h>

#define VCPort 25000

ref class VC : public NetworkedModule {
public:
    //Default Constructor 
    VC();

    // Overloaded Constructor
    VC(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC);

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

    void processHeartbeats();

    //destructor
    ~VC();

private:
    // Add any additional data members or helper functions here
    array<unsigned char>^ SendData;      // Array to store sensor data
    SM_VehicleControl^ SM_VehicleControl_;
    Stopwatch^ Watch;
    int flag;
};