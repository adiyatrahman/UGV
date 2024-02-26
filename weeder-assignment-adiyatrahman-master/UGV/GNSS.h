#pragma once

#include <NetworkedModule.h>

#define GNSSPort 24000

ref class GNSS : public NetworkedModule {
public:
    //Default constructor
    GNSS();

    //Overloaded Contructor 
    GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS);

    // Establish TCP connection
    error_state connect(String^ hostName, int portNumber) override;

    // Communicate over TCP connection (includes any error checking required on data)
    error_state communicate() override;

    //// Create shared memory objects
    //error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownThreads();

    // Get Shutdown signal for thread, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    error_state processHeartbeats();

    unsigned long CRC32Value(int i);
    unsigned long CalculateBlockCRC32(unsigned long ulCount, array<unsigned char>^ ucBuffer);

    //destructor
    ~GNSS();

private:
    // Add any additional data members or helper functions here
    array<unsigned char>^ SendData;      // Array to store sensor data
    array<unsigned char>^ ReadData;
    SM_GPS^ SM_GPS_;
    Stopwatch^ Watch;
};
