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
HGDIOBJ hGreenPen, hRedPen, hPinkPen, hLtGrnPen;
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
	hGreenPen = CreatePen(BS_SOLID, 1, RGB(0,0,0));
	hPinkPen = CreatePen(BS_SOLID, 1, RGB(200,100 ,100));
	hLtGrnPen = CreatePen(BS_SOLID, 1, RGB(127,127,127));
	hBackGround = CreateSolidBrush(RGB(200, 200, 200));
   ShowWindow(hWnd, SW_SHOWNORMAL);
   UpdateWindow(hWnd);
   CreateThread(NULL,8000, WorkerProc, NULL, 0, &ThreadID);
   return TRUE;
}

POINT LineToDraw[N_POINTS];

void DoPaint(HWND hWnd){
	HDC hdc, Memhdc;
	PAINTSTRUCT ps;
	HBITMAP Membitmap;
	RECT rc;
	long xScale, yScale, width, height, xOffset;
	WORD i,ii, floor;

	Painting = TRUE;
	while (Getting) Sleep(1);
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


	SelectObject(Memhdc, hGreenPen);
	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		LineToDraw[ii].x = xOffset - ((xScale * DataPoints[1 + i]) >> 16);
		LineToDraw[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[1 + i])) >> 16) + floor;
	}
	Polyline(Memhdc, LineToDraw, N_POINTS);


	SelectObject(Memhdc, hRedPen);
	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		LineToDraw[ii].x = xOffset - ((xScale * DataPoints[2 + i]) >> 16);
		LineToDraw[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[2 + i])) >> 16) + floor;
	}
	Polyline(Memhdc, LineToDraw, N_POINTS);

	if (dualDisplay) {
		xScale /= 2;
		yScale /= 2;
		xOffset -= width/2;
		floor -= height / 2;

		SelectObject(Memhdc, hPinkPen);
		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			LineToDraw[ii].x = xOffset - ((xScale * AltData[2 + i]) >> 16);
			LineToDraw[ii].y = ((yScale * (AltData[0 + i] - AltData[2 + i])) >> 16) + floor;
		}
		Polyline(Memhdc, LineToDraw, N_POINTS);

		SelectObject(Memhdc, hLtGrnPen);
		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			LineToDraw[ii].x = xOffset - ((xScale * AltData[1 + i]) >> 16);
			LineToDraw[ii].y = ((yScale * (AltData[0 + i] - AltData[1 + i])) >> 16) + floor;
		}
		Polyline(Memhdc, LineToDraw, N_POINTS);
	}

   //TextOut(Memhdc, 20, 20, L"Hello, Windows!", 16); 

	BitBlt(hdc, 0, 0, width, height, Memhdc, 0, 0, SRCCOPY);
	DeleteObject(Membitmap);
	DeleteDC(Memhdc);
	DeleteDC(hdc);
	EndPaint(hWnd, &ps);
	Painting = FALSE;
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

