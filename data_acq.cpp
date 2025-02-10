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
	if (!ReadFile(ioHandle, localBuf, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit(L"I/O error");
	if (sizeof(adcBuffer_t) == returnedReadings) {
		if (0 == (localBuf[0] & 0x8000)) { // look for sync flag
			Sleep(40);
			PurgeComm(ioHandle, PURGE_RXCLEAR);
			return false;
		}
		localBuf[0] &= ~0x8000; // clear that flag 
		WaitForSingleObject(hMutex, INFINITE);
		memcpy(buf, localBuf, sizeof(adcBuffer_t));
		ReleaseMutex(hMutex);
		return true;
	}
	stalls++;
	Sleep(40);
	PurgeComm(ioHandle, PURGE_RXCLEAR);
	false;
}

void GetData(void) {
	
	while (!Stopping) {
		if (!GrabAFrame(DataPoints, 'T'))
			continue;
		if (dualDisplay) {
			if (!GrabAFrame(AltData, 'W'))
				continue;
		}
		InvalidateRect(hWnd, 0, FALSE);
		continue;
	}


	//Stop the stream
	CloseHandle(ioHandle);
	Stopped = true;
}
