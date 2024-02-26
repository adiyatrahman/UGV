#pragma once

#include "ControllerInterface.h"
#include <UGVModule.h>

ref class Controller : public UGVModule {
public:
    //Default Contructor 
    Controller();

    //Overloaded Contructor 
    Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC);

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownThreads();

    // Get Shutdown signal for thread, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    void processHeartbeats();

    //destructor
    ~Controller();

private:
    // Add any additional data members or helper functions here
    SM_VehicleControl^ SM_VehicleControl_;
    Stopwatch^ Watch;
    ControllerInterface* newInterface;
    controllerState current_state;
};