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
	DWORD written;
	ioHandle = FindCommPort();
	//ioHandle = CreateFile(L"COM6", (GENERIC_READ | GENERIC_WRITE), 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ioHandle == INVALID_HANDLE_VALUE) Damnit();
	WriteFile(ioHandle, L"S", 1, &written, NULL);
	if (written != 1) Damnit();
}




DWORD WINAPI WorkerProc(LPVOID lpParam) {
	InitComms();
	GetData();
	ExitThread(0);
}

void GetData(void){
    DWORD returnedReadings;
	unsigned Counter = 0;

    while (!Stopping) {
		if (Painting) {
			Sleep(1);
			continue;
		}
		WriteFile(ioHandle, "T", 1, &returnedReadings, NULL);
		if (!ReadFile(ioHandle, DataPoints, sizeof(adcBuffer_t), &returnedReadings, NULL)) Damnit();
		Counter = sizeof(adcBuffer_t);
		if (sizeof(adcBuffer_t) == returnedReadings)
			InvalidateRect(hWnd, 0, TRUE);
		else
			Sleep(5);


#if 0
		if (Counter < 5){
			ERR_WRAP(eGet(hLJ, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS, &returnedReadings, (long)&DataPoints[0]));
			InvalidateRect(hWnd, 0, TRUE);
		}

		if (dualDisplay && ++Counter >= 4){
			Counter = 0;
			ePut(hLJ, LJ_ioPUT_DIGITAL_BIT, 16, 0, 0);
			ERR_WRAP(eGet(hLJ, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS, &returnedReadings, (long)&AltData[0]));
			ERR_WRAP(eGet(hLJ, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS, &returnedReadings, (long)&AltData[0]));
			// high drive
			ePut(hLJ, LJ_ioPUT_DIGITAL_BIT, 16, 1, 0);
			ERR_WRAP(eGet(hLJ, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS, &returnedReadings, (long)&DataPoints[0]));
			ERR_WRAP(eGet(hLJ, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS, &returnedReadings, (long)&DataPoints[0]));
			InvalidateRect(hWnd, 0, TRUE);
		}
#endif

	}

   
	//Stop the stream
	CloseHandle(ioHandle);
	Stopped = true;
}
