#using <System.dll>
#include "Display.h"

Display::Display() {}

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_GPS_ = SM_Gps;
	Watch = gcnew Stopwatch;
}

error_state Display::connect(String^ hostName, int portNumber) {
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client-> ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(64);
	return SUCCESS;
}

// Communicate over TCP connection (includes any error checking required on data)
error_state Display::communicate() {
	sendDisplayData();
	return SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state Display::processSharedMemory() {
	return SUCCESS;
}

void Display::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
}

// Get Shutdown signal for thread, from Thread Management SM
bool Display::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_DISPLAY);
}

// Thread function for TMM
void Display::threadFunction() {
	Console::WriteLine("Display thread is starting");
	connect(DISPLAY_ADDRESS, DisplayPort);
	//wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//Start the stopwatch
	Watch->Start();
	//Start the thread loop
	while (!getShutdownFlag()) {
		Console::WriteLine("Display thread is running.");
		processHeartbeats();
		if (communicate() == SUCCESS) {
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("GPS thread is terminating");
	return;
}

void Display::processHeartbeats() {
	//Is the Display bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_DISPLAY) == 0) {
		//put the Display bit up
		SM_TM_->heartbeat |= bit_DISPLAY;
		//reset the stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			Console::WriteLine("Display->TMM Failed. Critical Thread.");
			shutdownThreads();
		}
	}
}
//Destructor
Display:: ~Display() {
	return;
}

void Display::sendDisplayData() {
	// Serialize the data arrays to a byte array
	//(format required for sending)
	
	// Lock SM
	Monitor::Enter(SM_Laser_->lockObject);

	array<Byte>^ dataX =
		gcnew array<Byte>(SM_Laser_->x->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
	array<Byte>^ dataY =
		gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);

	//unlock SM
	Monitor::Exit(SM_Laser_->lockObject);

	// Send byte data over connection
	Stream->Write(dataX, 0, dataX->Length);
	Thread::Sleep(10);
	Stream->Write(dataY, 0, dataY->Length);
}
