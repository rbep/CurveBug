/***************************************************************************
 *
 *  Project  CurveBug
 *
 * Copyright (C) Robert Puckette, 2024-2025
 *
 * This software is licensed as described in the file COPYING.txt, which
 * you should have received as part of this distribution.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *
 ***************************************************************************/
#include "stdafx.h"
#include "curvebug.h"
#include "UsbFind.h"


adcBuffer_t DataPoints;
adcBuffer_t AltData;
HANDLE ioHandle;
BYTE DeviceID[4];


void InitComms() {
	DWORD readSize;
	int retVal;
	ioHandle = FindCommPort();
	if (ioHandle == INVALID_HANDLE_VALUE) Damnit(L"Couldn't Find Device");

	char cmd = '?';

	WriteFile(ioHandle, &cmd, 1, &readSize, NULL);

	// get resultant data
	if (!ReadFile(ioHandle, DeviceID, sizeof(DeviceID), &readSize, NULL))
		Damnit(L"CurveBug device not communicating");
	if (DeviceID[3] > 2 ||
		DeviceID[2] > 3) {
		retVal = MessageBox(hWnd,
			L"A possible incompatibily exists between this application and the attached CurveBug. "
			"You should consider updating this application. Do you wish to continue?",
			L"Potential Issue",
			MB_SYSTEMMODAL | MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
		if (retVal == IDNO)
			exit(1);
	}

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
