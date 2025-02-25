#include "stdafx.h"
#include "curvebug.h"
#include "UsbFind.h"


adcBuffer_t DataPoints;
adcBuffer_t AltData;
HANDLE ioHandle;
BYTE DeviceID[4];


void InitComms() {
	DWORD readSize;
	ioHandle = FindCommPort();
	if (ioHandle == INVALID_HANDLE_VALUE) Damnit(L"Couldn't Find Device");

	char cmd = '?';

	WriteFile(ioHandle, &cmd, 1, &readSize, NULL);

	// get resultant data
	if (!ReadFile(ioHandle, DeviceID, sizeof(DeviceID), &readSize, NULL))
		Damnit(L"I/O error");
	if (DeviceID[3] != 1)
		Damnit(L"Unrecognized Device");
	if (DeviceID[2] > 1)
		Damnit(L"Device needs new rev of SW");

}




bool GrabAFrame(adcBuffer_t buf, char cmd) {
	DWORD returnedReadings;
	adcBuffer_t localBuf;

	// send a ADC trigger command
	WriteFile(ioHandle, &cmd, 1, &returnedReadings, NULL);

	// get resultant data
	if (!ReadFile(ioHandle, localBuf, sizeof(adcBuffer_t), &returnedReadings, NULL))
		Damnit(L"I/O error");

	// validate the goodness of the returned data
	if (sizeof(adcBuffer_t) == returnedReadings) {
		if (0 == (localBuf[0] & 0x8000)) { // look for sync flag
			// wait a bit... clear pending data and return with failure
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
	// wait a bit... clear pending data and return with failure
	Sleep(40);
	PurgeComm(ioHandle, PURGE_RXCLEAR);
	return false;
}

// Data acquisition loop
void GetData(void) {

	while (!Stopping) {
		// if we're paused, block the update
		if (Hold) {
			Sleep(100);
			continue;
		}
		if (!GrabAFrame(DataPoints, DriveMode != weak ? 'T' : 'W')) // trigger single scan
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

DWORD WINAPI WorkerProc(LPVOID lpParam) {
	InitComms();
	GetData();
	ExitThread(0);
}
