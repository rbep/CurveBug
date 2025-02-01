#include "stdafx.h"
#include "curvebug.h"
#include "UsbFind.h"


adcBuffer_t DataPoints;
adcBuffer_t AltData;
HANDLE ioHandle;

void Damnit(void);
void GetData(void);
bool Stopped = false;
bool Stopping = false;


void InitComms(){
	ioHandle = FindCommPort();
	if (ioHandle == INVALID_HANDLE_VALUE) Damnit();
}


DWORD WINAPI WorkerProc(LPVOID lpParam) {
	InitComms();
	GetData();
	ExitThread(0);
}

void GetData(void){
    DWORD returnedReadings;

    while (!Stopping) {
		if (Painting) {
			Sleep(1);
			continue;
		}
		WriteFile(ioHandle, "T", 1, &returnedReadings, NULL);
		Sleep(1);
		if (!ReadFile(ioHandle, DataPoints, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit();
		if (sizeof(adcBuffer_t) == returnedReadings)
			InvalidateRect(hWnd, 0, FALSE);
		else
			Sleep(5);
		if (dualDisplay) {
			WriteFile(ioHandle, "W", 1, &returnedReadings, NULL);
			Sleep(1);
			if (!ReadFile(ioHandle, AltData, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit();
			if (sizeof(adcBuffer_t) == returnedReadings)
				InvalidateRect(hWnd, 0, FALSE);
			else
				Sleep(5);
		}

	}

   
	//Stop the stream
	CloseHandle(ioHandle);
	Stopped = true;
}
