#pragma once

#include "resource.h"

#define ADC_MAX 2800L
#define N_POINTS (1512/3)
typedef WORD adcBuffer_t[N_POINTS * 3];

extern adcBuffer_t DataPoints, AltData;
extern HWND hWnd;
extern bool Stopped;
extern bool Stopping;
extern bool dualDisplay;
extern bool Painting;
extern bool Getting;

DWORD WINAPI WorkerProc(LPVOID lpParam);
void Damnit(wchar_t *msg);
