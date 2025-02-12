#pragma once

#include "resource.h"

#define ADC_MAX 2400L
#define N_POINTS (1512/3)
//#define N_POINTS (1002/3)  //jjjjj

#define DIM(a) (sizeof(a)/sizeof(a[0]))
typedef WORD adcBuffer_t[N_POINTS * 3];

extern adcBuffer_t DataPoints, AltData;
extern HWND hWnd;
extern bool Stopped;
extern bool Stopping;
extern bool dualDisplay;
extern HANDLE hMutex;
extern DWORD stalls;
extern DWORD scans;

DWORD WINAPI WorkerProc(LPVOID lpParam);
void Damnit(wchar_t *msg);
