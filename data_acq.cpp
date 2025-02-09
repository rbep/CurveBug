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

void GetData(void) {
	DWORD returnedReadings;
	adcBuffer_t localBuf;
	bool needRepaint;
	DWORD ticks;

	while (!Stopping) {
		needRepaint = false;
		WriteFile(ioHandle, "T", 1, &returnedReadings, NULL);
		ticks = GetTickCount();
		if (!ReadFile(ioHandle, localBuf, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit(L"I/O error");
		ticks = GetTickCount() - ticks;
		if (sizeof(adcBuffer_t) == returnedReadings) {
			needRepaint = true;
			WaitForSingleObject(hMutex, INFINITE);
			memcpy(DataPoints, localBuf, sizeof(adcBuffer_t));
			ReleaseMutex(hMutex);
		}
		else {
			stalls++;
			Sleep(40);
			PurgeComm(ioHandle, PURGE_RXCLEAR);
			continue;
		}
		if (dualDisplay) {
			WriteFile(ioHandle, "W", 1, &returnedReadings, NULL);
 			if (!ReadFile(ioHandle, localBuf, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit(L"I/O error");
			if (sizeof(adcBuffer_t) == returnedReadings) {
				needRepaint = true;
				WaitForSingleObject(hMutex, INFINITE);
				memcpy(AltData, localBuf, sizeof(adcBuffer_t));
				ReleaseMutex(hMutex); 
			}
			else {
				stalls++;
				Sleep(40);
				PurgeComm(ioHandle, PURGE_RXCLEAR);
				continue;
			}
		}
		if (needRepaint)
			InvalidateRect(hWnd, 0, FALSE);
	}


	//Stop the stream
	CloseHandle(ioHandle);
	Stopped = true;
}
