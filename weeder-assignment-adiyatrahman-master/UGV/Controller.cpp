#include "Controller.h"
#using <System.dll>

//Default Constructor
Controller::Controller() {}

//Overloaded constructor
Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_VehicleControl_ = SM_VC;
	Watch = gcnew Stopwatch;
	newInterface = new ControllerInterface(1,0);
}

// Send/Recieve data from shared memory structures
error_state Controller::processSharedMemory() {
	if (!newInterface->IsConnected()) {
		// Unlock SM
		Monitor::Enter(SM_VehicleControl_->lockObject);
		SM_VehicleControl_->Speed = 0;
		SM_VehicleControl_->Steering = 0;

		//Lock SM
		Monitor::Exit(SM_VehicleControl_->lockObject);
		return ERR_CONNECTION;
	}

	//Console::WriteLine("Controller Status: ");
	current_state = newInterface->GetState();
	// newInterface->printControllerState(current_state);
	// Console::WriteLine();

	double forward_motion = current_state.rightTrigger * 1;
	double reverse_motion = current_state.leftTrigger * 1;
	double steering_angle = current_state.rightThumbX * -40;

	// Unlock SM
	Monitor::Enter(SM_VehicleControl_->lockObject);
	SM_VehicleControl_->Speed = forward_motion - reverse_motion;
	SM_VehicleControl_->Steering = steering_angle;

	//Lock SM
	Monitor::Exit(SM_VehicleControl_->lockObject);
	
	if (current_state.buttonA) {
		shutdownThreads();
		Console::WriteLine("Button A has been pressed to turn off the threads");
	}
	return SUCCESS;
};

void Controller::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
};

// Get Shutdown signal for thread, from Thread Management SM
bool Controller::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_CONTROLLER);
};

// Thread function for TMM
void Controller::threadFunction() {
	Console::WriteLine("Controller thread is starting");
	//wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//Start the stopwatch
	Watch->Start();
	//Start the thread loop
	while (!getShutdownFlag()) {
		Console::WriteLine("Controller thread is running.");
		processHeartbeats();
		processSharedMemory();
		Thread::Sleep(20);
	}
	Console::WriteLine("Controller thread is terminating");
	return;
};

void Controller::processHeartbeats() {
	//Is the CONTROLLER bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0) {
		//put the CONTROLLER bit up
		SM_TM_->heartbeat |= bit_CONTROLLER;
		//reset the stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			Console::WriteLine("Controller->TMM Failed. Critical Thread.");
			shutdownThreads();
		}
	}
}

//Destructor
Controller:: ~Controller() {
	return;
}