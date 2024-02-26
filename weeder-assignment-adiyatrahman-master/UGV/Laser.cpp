#using <System.dll>
#include "Laser.h"

//Default Contructor
Laser::Laser() {};


//Overloaded Contructor 
Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	Watch = gcnew Stopwatch;
}

//Establish TCP connection
error_state Laser::connect(String^ hostName, int portNumber) {
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client-> ReceiveBufferSize = 1024;
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

// Communicate over TCP connection (includes any error checking required on data)
error_state Laser::communicate() {
	// command
	String^ Command = "sRN LMDscandata";
	// send STX byte 0x02
	Stream->WriteByte(0x02);
	// send command
	SendData = Encoding::ASCII->GetBytes(Command);
	Stream->Write(SendData, 0, SendData->Length);
	// send ETX byte 0x03
	Stream->WriteByte(0x03);
	// wait for some time or use DataAvailable
	System::Threading::Thread::Sleep(10);
	// read the data from the laser range finder
	//Stream->Read(ReadData, 0, ReadData->Length);
	return SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state Laser::processSharedMemory() {
	int NumPoints;
	Stream->Read(ReadData, 0, ReadData->Length);
	String^ Response = Encoding::ASCII->GetString(ReadData);
	//check if the total number of fields have been received
	array<String^>^ Fragments;
	Fragments = Response->Split(' ');

	NumPoints = Convert::ToInt32(Fragments[25], 16);
	double Resolution = Convert::ToInt32(Fragments[24], 16) / 10000.0;

	// unlock SM
	Monitor::Enter(SM_Laser_->lockObject);

	//Write to SM
	for (int i = 0; i < NumPoints; i++) {
		SM_Laser_->x[i] = (Convert::ToInt32(Fragments[26 + i], 16)) * Math::Sin(i * Resolution * Math::PI / 180.0);
		SM_Laser_->y[i] = - ((Convert::ToInt32(Fragments[26 + i], 16)) * Math::Cos(i * Resolution * Math::PI / 180.0));

		//Console::Write("X: ");
		//Console::Write(SM_Laser_->x[i]);
		//Console::Write(", Y: ");
		//Console::WriteLine(SM_Laser_->y[i]);
	}

	//Lock SM
	Monitor::Exit(SM_Laser_->lockObject);

	if (Fragments->Length == 400) {
		return SUCCESS;
	}
	else {
		return ERR_INVALID_DATA;
	}
}

void Laser::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
}

// Get Shutdown signal for thread, from Thread Management SM
bool Laser::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_LASER);
}

// Thread function for TMM

void Laser::threadFunction() {
	Console::WriteLine("Laser thread is starting");
	connect(WEEDER_ADDRESS, LaserPort);
	//wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//Start the stopwatch
	Watch->Start();
	//Start the thread loop
	
	int i = 0;
	while (!getShutdownFlag()) {
		Console::WriteLine("Laser thread is running.");
		processHeartbeats();
		if (communicate() == SUCCESS) {
			processSharedMemory();
		}
		Thread::Sleep(50);

		//if (i++ > 100) break;
	}
	Console::WriteLine("Laser thread is terminating");
	return;
}

void Laser::processHeartbeats() {
	//Is the laser bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_LASER) == 0) {
		//put the laser bit up
		SM_TM_->heartbeat |= bit_LASER;
		//reset the stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			Console::WriteLine("Laser->TMM Failed. Critical Thread.");
			shutdownThreads();
		}
	}
}

//Destructor
Laser:: ~Laser() {
	return;
}