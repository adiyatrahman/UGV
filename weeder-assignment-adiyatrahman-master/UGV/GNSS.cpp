#using <System.dll>
#include "GNSS.h"


#define CRC32_POLYNOMIAL 0xEDB88320L

//Default Constructor
GNSS::GNSS() {}

//Overloaded Contructor 
GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS) {
	SM_TM_ = SM_TM;
	SM_GPS_ = SM_GPS;
	Watch = gcnew Stopwatch;
}

//Establish TCP connection
error_state GNSS::connect(String^ hostName, int portNumber) {
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(2048);
	//Thread::Sleep(5);
	return SUCCESS;
}

// Communicate over TCP connection (includes any error checking required on data)
error_state GNSS::communicate() {
	while (!Stream->DataAvailable) {}
	unsigned int Header = 0;
	double Northing, Easting, height;
	Byte Data;

	do {
		Data = Stream->ReadByte();
		Header = (Header << 8) | Data;
	} while (Header != 0xAA44121C);

	ReadData[0] = 0xaa;
	ReadData[1] = 0x44;
	ReadData[2] = 0x12;
	ReadData[3] = 0x1c;
	
	return SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state GNSS::processSharedMemory() {
	while (!Stream->DataAvailable) {}
	Stream->Read(ReadData, 4, ReadData->Length - 4);

	unsigned long Checksum;
	unsigned long CalcSum;

	Checksum = BitConverter::ToInt32(ReadData, 108);
	CalcSum = CalculateBlockCRC32(108, ReadData);

	if (Checksum == CalcSum) {
		Console::WriteLine("Checksum match");
		// unlock SM
		Monitor::Enter(SM_GPS_->lockObject);

		SM_GPS_->Northing = BitConverter::ToDouble(ReadData, 44);
		SM_GPS_->Easting = BitConverter::ToDouble(ReadData, 52);
		SM_GPS_->Height = BitConverter::ToDouble(ReadData, 60);

		Console::Write("Northing: ");
		Console::Write(SM_GPS_->Northing);

		Console::Write("; Easting: ");
		Console::Write(SM_GPS_->Easting);

		Console::Write("; Height: ");
		Console::Write(SM_GPS_->Height);

		Console::Write("; Checksum: ");
		Console::WriteLine("{0:X8}", Checksum);

		//Lock SM
		Monitor::Exit(SM_GPS_->lockObject);
		return SUCCESS;
	}
	else {
		return ERR_INVALID_DATA;
	}

	return SUCCESS;
}

void GNSS::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
}

// Get Shutdown signal for thread, from Thread Management SM
bool GNSS::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_GPS);
}

// Thread function for TMM
void GNSS::threadFunction() {
	Console::WriteLine("GNSS thread is starting");
	connect(WEEDER_ADDRESS, GNSSPort);
	//wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//Start the stopwatch
	Watch->Start();
	//Start the thread loop
	int i = 0;
	while (!getShutdownFlag()) {
		Console::WriteLine("GNSS thread is running.");
		processHeartbeats();
		if (communicate() == SUCCESS) {
			processSharedMemory();
		}
		Thread::Sleep(1000);
		//if (i++ > 10) break;
	}
	Console::WriteLine("GPS thread is terminating");
	return;
}

error_state GNSS::processHeartbeats() {
	//Is the GPS bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_GPS) == 0) {
		//put the GPS bit up
		SM_TM_->heartbeat |= bit_GPS;
		//reset the stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			Console::WriteLine("GPS->TMM Failed. Critical Thread.");
			shutdownThreads();
			return ERR_TMT_FAILURE;
		}
	}
	return SUCCESS;
}

//Destructor
GNSS:: ~GNSS() {
	return;
}

unsigned long GNSS::CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}
unsigned long GNSS::CalculateBlockCRC32(
	unsigned long ulCount, /* Number of bytes in the data block */
	array<unsigned char>^ ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	int i{ 0 };
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ ucBuffer[i++]) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}
