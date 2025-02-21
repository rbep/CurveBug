#pragma once

#include "resource.h"

#define ADC_MAX 2800L
#define N_POINTS (1008/3)
#define BACKGROUND_COLOR RGB(200, 200, 200)

#define DIM(a) (sizeof(a)/sizeof(a[0]))
typedef WORD adcBuffer_t[N_POINTS * 3];

extern adcBuffer_t DataPoints, AltData;
extern HWND hWnd;
extern bool Hold;
extern bool Stopped;
extern bool Stopping;
extern HANDLE hMutex;
extern DWORD scans;

enum  Mode { strong, weak, dual };
extern Mode DriveMode;

DWORD WINAPI WorkerProc(LPVOID lpParam);
void Damnit(wchar_t *msg);
