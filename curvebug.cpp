// curve3.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "curvebug.h"
#include "UsbFind.h"

#define MAX_LOADSTRING 100

// Global Variables:

HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
bool Stopped = false;
bool Stopping = false;
DWORD stalls = 0;
HANDLE hMutex;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	hMutex = CreateMutex(NULL, FALSE, NULL);
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CURVEBUG, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CURVEBUG));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CURVEBUG));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= L""; //MAKEINTRESOURCE(IDC_CURVE3);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDC_CURVEBUG));

	return RegisterClassEx(&wcex);
}

DWORD ThreadID;
//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND hWnd;
HGDIOBJ hBlackPen, hRedPen, hPinkPen, hGrayPen;
HBRUSH hBackGround;
bool dualDisplay;
bool Painting = FALSE;
bool Getting = FALSE;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
	hRedPen = CreatePen(BS_SOLID, 1, RGB(255,0,0));
	hBlackPen = CreatePen(BS_SOLID, 1, RGB(0,0,0));
	hPinkPen = CreatePen(BS_SOLID, 1, RGB(200,100 ,100));
	hGrayPen = CreatePen(BS_SOLID, 1, RGB(127,127,127));
	hBackGround = CreateSolidBrush(RGB(200, 200, 200));
   ShowWindow(hWnd, SW_SHOWNORMAL);
   UpdateWindow(hWnd);
   CreateThread(NULL,8000, WorkerProc, NULL, 0, &ThreadID);
   return TRUE;
}

POINT BlackLine[N_POINTS];
POINT RedLine[N_POINTS];

void DoPaint(HWND hWnd){
	HDC hdc, Memhdc;
	PAINTSTRUCT ps;
	HBITMAP Membitmap;
	RECT rc;
	long xScale, yScale, width, height, xOffset;
	WORD i,ii, floor;

	GetClientRect(hWnd, &rc);

	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
	floor = height * 7/ 8 + rc.top;
	xOffset = (rc.left + width - 1);
	xScale = ((DWORD)width << 16) / ADC_MAX;
    yScale = (height << 16) / (ADC_MAX - 700);
	hdc = BeginPaint(hWnd, &ps);
	hdc = GetDC(hWnd);
	Memhdc = CreateCompatibleDC(hdc);
	Membitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(Memhdc, Membitmap);
	FillRect(Memhdc, &rc, hBackGround);
	SetBkColor(Memhdc, RGB(200, 200, 200));


	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		BlackLine[ii].x = xOffset - ((xScale * DataPoints[1 + i]) >> 16);
		BlackLine[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[1 + i])) >> 16) + floor;
	}

	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		RedLine[ii].x = xOffset - ((xScale * DataPoints[2 + i]) >> 16);
		RedLine[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[2 + i])) >> 16) + floor;
	}
	WaitForSingleObject(hMutex, INFINITE);
	SelectObject(Memhdc, hBlackPen);
	Polyline(Memhdc, BlackLine, N_POINTS);
	SelectObject(Memhdc, hRedPen);
	Polyline(Memhdc, RedLine, N_POINTS);
	ReleaseMutex(hMutex);

	if (dualDisplay) {
		xScale = xScale * 3 / 4;
		yScale = yScale * 3 / 4;
		xOffset -= width/4;
		floor -= height / 4;

		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			BlackLine[ii].x = xOffset - ((xScale * AltData[2 + i]) >> 16);
			BlackLine[ii].y = ((yScale * (AltData[0 + i] - AltData[2 + i])) >> 16) + floor;
		}

		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			RedLine[ii].x = xOffset - ((xScale * AltData[1 + i]) >> 16);
			RedLine[ii].y = ((yScale * (AltData[0 + i] - AltData[1 + i])) >> 16) + floor;
		}
		WaitForSingleObject(hMutex, INFINITE);
		SelectObject(Memhdc, hPinkPen);
		Polyline(Memhdc, BlackLine, N_POINTS);
		SelectObject(Memhdc, hGrayPen);
		Polyline(Memhdc, RedLine, N_POINTS);
		ReleaseMutex(hMutex);
	}
	
	TCHAR text[20];
	_itot(stalls, text, 10);
    TextOut(Memhdc, 20, 20, text, _tcsclen(text));

	BitBlt(hdc, 0, 0, width, height, Memhdc, 0, 0, SRCCOPY);
	DeleteObject(Membitmap);
	DeleteDC(Memhdc);
	DeleteDC(hdc);
	EndPaint(hWnd, &ps);
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		dualDisplay= !dualDisplay;
		break;
	case WM_LBUTTONDOWN:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		break;
	case WM_PAINT:
		DoPaint(hWnd);
		break;
	case WM_DESTROY:
		Stopped = true;
		for (int i = 0; i < 10 && !Stopped; i++)
			Sleep(50);
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		return 1;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}




// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void Damnit(wchar_t *msg) {
	WCHAR errMsg[128];

	if (msg == NULL) {
		msg = errMsg;
		_wcserror_s(errMsg, sizeof(errMsg) / sizeof(*errMsg), GetLastError());
	}
	MessageBox(NULL, msg, L"Bummer!", MB_SYSTEMMODAL);
	exit(1);
}

