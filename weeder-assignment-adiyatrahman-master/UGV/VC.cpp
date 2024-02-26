#using <System.dll>
#include "VC.h"

// Default Contructor 
VC::VC() {}

// Overloaded Contructor 
VC::VC(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_VehicleControl_ = SM_VC;
	Watch = gcnew Stopwatch;
	flag = 0;
}

error_state VC::connect(String^ hostName, int portNumber) {
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(2048);

	// Authentication command
	String^ Command = "5346887\n";
	SendData = Encoding::ASCII->GetBytes(Command);
	Stream->Write(SendData, 0, SendData->Length);
	Thread::Sleep(50);

	while (!Stream->DataAvailable) {}
	Stream->Read(ReadData, 0, ReadData->Length);
	String^ Response = Encoding::ASCII->GetString(ReadData);

	if (Response->Contains("OK\n")) {
		Console::WriteLine("Successfully Authenticated");
		return SUCCESS;
	}
	else {
		Console::WriteLine("Error");
		return ERR_AUTHENTICATION;
	}
	return SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state VC::processSharedMemory() {
	return SUCCESS;
}

// Communicate over TCP connection (includes any error checking required on data)
error_state VC::communicate() {
	double curr_speed = 0;
	double curr_steering = 0;

	// Unlock SM
	Monitor::Enter(SM_VehicleControl_->lockObject);
	curr_speed = SM_VehicleControl_->Speed;
	curr_steering = SM_VehicleControl_->Steering;
	//Lock SM
	Monitor::Exit(SM_VehicleControl_->lockObject);

	// Form the string using String::Format
	String^ Command = String::Format("# {0} {1} {2} #", curr_steering, curr_speed, flag);

	// send command
	SendData = Encoding::ASCII->GetBytes(Command);
	Stream->Write(SendData, 0, SendData->Length);

	// Toggle the flag from 0 to 1 or vice versa
	flag = (flag == 0) ? 1 : 0;

	Thread::Sleep(10);
	return SUCCESS;
}

void VC::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
}

// Get Shutdown signal for thread, from Thread Management SM
bool VC::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_VC);
}

// Thread function for TMM
void VC::threadFunction() {
	Console::WriteLine("VC thread is starting");
	connect(WEEDER_ADDRESS, VCPort);
	//wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//Start the stopwatch
	Watch->Start();
	//Start the thread loop
	while (!getShutdownFlag()) {
		Console::WriteLine("VC thread is running.");
		processHeartbeats();
		if (communicate() == SUCCESS) {
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("VC thread is terminating");
	return;
}

void VC::processHeartbeats() {
	//Is the VC bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_VC) == 0) {
		//put the VC bit up
		SM_TM_->heartbeat |= bit_VC;
		//reset the stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			Console::WriteLine("VC->TMM Failed. Critical Thread.");
			shutdownThreads();
		}
	}
}

//Destructor
VC:: ~VC() {
	return;
}