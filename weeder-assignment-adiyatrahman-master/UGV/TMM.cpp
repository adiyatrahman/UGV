#include "Controller.h"
#using <System.dll>

#include "TMM.h"
#include "Laser.h"
#include "GNSS.h"
#include "VC.h"
#include "Display.h"
#include <conio.h>

// Create shared memory objects
error_state ThreadManagement::setupSharedMemory() {
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_GPS_ = gcnew SM_GPS;
	SM_VehicleControl_ = gcnew SM_VehicleControl;

	return SUCCESS;
};

// Send/Recieve data from shared memory structures
error_state ThreadManagement::processSharedMemory() {
	// check the heartbeat flag of the ith thread (is it high?)
	for (int i = 0; i < ThreadList->Length; i++) {
		// if high put ith bit (flag) down 
		if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID) {
			//put the flag down
			SM_TM_->heartbeat ^= ThreadPropertiesList[i]->BitID;
			// Reset the stopwatch
			StopwatchList[i]->Restart();
		}
		else
		{
			// check the stopwatch. Is time exceeded crash time limit?
			if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT) {
				//is ith process a critical process?
				if (ThreadPropertiesList[i]->Critical) {
					//shutdown all
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " FAILURE. Shutting down all threads.");
					shutdownThreads();
					return ERR_CRITCAL_PROCESS_FAILURE;
				}
				//else
				else
				{
					//try to restart
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " FAILURE. Attempting to restart.");
					ThreadList[i]->Abort();
					ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
					SM_TM_->ThreadBarrier = gcnew Barrier(1);
					ThreadList[i]->Start();
					Thread::Sleep(50);
				}
			}
		}
			
	}
	return SUCCESS;
};

void ThreadManagement::shutdownThreads() {
	SM_TM_->shutdown = 0xFF;
	return;
};

// Get Shutdown signal for thread, from Thread Management SM
bool ThreadManagement::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_TM);
};

// Thread function for TMM
void ThreadManagement::threadFunction() {
	Console::WriteLine("TMT thread is starting");
	//make a list of thread properties
	ThreadPropertiesList = gcnew array<ThreadProperties^>{
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_), &Laser::threadFunction), true, bit_LASER, "Laser thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GPS_), &GNSS::threadFunction), false, bit_GPS, "GPS thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew VC(SM_TM_, SM_VehicleControl_), &VC::threadFunction), true, bit_VC, "Vehicle Control thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_, SM_GPS_), &Display::threadFunction), true, bit_DISPLAY, "Display thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_VehicleControl_), &Controller::threadFunction), true, bit_CONTROLLER, "Controller thread")
	};

	//make a list of threads
	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);
	// make the stop watch list
	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);
	//make thread barrier
	SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);
	// start all the threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		StopwatchList[i] = gcnew Stopwatch;
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		ThreadList[i]->Start();
	}
	// wait at the TMT thread barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	//start all the stop watches
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		StopwatchList[i]->Start();
	}
	// start the thread loop 
		//keep checking the heartbeats
	char ch;
	while (!getShutdownFlag()) {
		Console::WriteLine("TMM Thread is running");
		processSharedMemory();
		Thread::Sleep(50);

		if (kbhit()) {
			ch = getch();
			if (ch == 'q' || ch == 'Q')
			{
				break;
			}
		}
	}
	// end of thread loop
	// shutdown threads
	shutdownThreads();
	// join all threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		ThreadList[i]->Join();
	}
	Console::WriteLine("TMM thread termination...");
	return;
};