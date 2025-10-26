#pragma once
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
#include "resource.h"

#define VERSION_STRING L"1.05"

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
extern BYTE DeviceID[4]; // bytes 1:0 are serial number, 2 - rev level, 3 - device type

enum  Mode { strong, weak, dual };
extern Mode DriveMode;

DWORD WINAPI WorkerProc(LPVOID lpParam);
void Damnit(PTCHAR msg);
