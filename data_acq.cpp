#include "stdafx.h"
#include "curvebug.h"
#include "UsbFind.h"


adcBuffer_t DataPoints;
adcBuffer_t AltData;
HANDLE ioHandle;

void GetData(void);


void InitComms(){
	ioHandle = FindCommPort();
	if (ioHandle == INVALID_HANDLE_VALUE) Damnit(L"Couldn't Find Device");
}


DWORD WINAPI WorkerProc(LPVOID lpParam) {
	InitComms();
	GetData();
	ExitThread(0);
}

bool GrabAFrame(adcBuffer_t buf, char cmd){
	DWORD returnedReadings;
	adcBuffer_t localBuf;

	WriteFile(ioHandle, &cmd, 1, &returnedReadings, NULL);
	if (!ReadFile(ioHandle, localBuf, sizeof(adcBuffer_t), &returnedReadings, NULL)) 
		Damnit(L"I/O error");
	if (sizeof(adcBuffer_t) == returnedReadings) {
		if (0 == (localBuf[0] & 0x8000)) { // look for sync flag
			Sleep(40);
			PurgeComm(ioHandle, PURGE_RXCLEAR);
			return false;
		}
		localBuf[0] &= ~0x8000; // clear that flag 

		// block display update while we fill with new data
		WaitForSingleObject(hMutex, INFINITE);
		memcpy(buf, localBuf, sizeof(adcBuffer_t));
		ReleaseMutex(hMutex);
		return true;
	}
	Sleep(40);
	PurgeComm(ioHandle, PURGE_RXCLEAR);
	return false;
}

// Data acquisition loop
void GetData(void) {
	
	while (!Stopping) {
		if (Hold) {
			Sleep(100);
			continue;
		}
		if (!GrabAFrame(DataPoints, DriveMode != weak ? 'T' : 'W')) // trigger strong scan
			continue;
		if (DriveMode == dual) {
			if (!GrabAFrame(AltData, 'W')) // trigger weak scan
				continue;
		}
		scans++;
		InvalidateRect(hWnd, 0, FALSE); // new data... signal repaint need
		Sleep(2);
		continue;
	}


	//Stop the stream
	CloseHandle(ioHandle);
	Stopped = true;
}
