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
bool Hold = false;								// just pause the data
bool SingleTrace = false;						// only one trace
DWORD scans = 0;
HANDLE hMutex;
Mode DriveMode = strong;

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

	hMutex = CreateMutex(NULL, FALSE, NULL);
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CURVEBUG, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
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

	return (int)msg.wParam;
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

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CURVEBUG));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.hbrBackground = CreateSolidBrush(BACKGROUND_COLOR);
	wcex.lpszMenuName = L""; //MAKEINTRESOURCE(IDC_CURVE3);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDC_CURVEBUG));

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
HGDIOBJ hBlackPen, hRedPen, hPinkPen, hGrayPen, hGreenPen;
HBRUSH hBackGround;
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
	hRedPen = CreatePen(BS_SOLID, 1, RGB(255, 0, 0));
	hBlackPen = CreatePen(BS_SOLID, 1, RGB(0, 0, 0));
	hPinkPen = CreatePen(BS_SOLID, 1, RGB(200, 100, 100));
	hGrayPen = CreatePen(BS_SOLID, 1, RGB(127, 127, 127));
	hGreenPen = CreatePen(BS_SOLID, 1, RGB(0, 200, 0));
	hBackGround = CreateSolidBrush(BACKGROUND_COLOR);
	SetBkColor(GetDC(hWnd), BACKGROUND_COLOR);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// launch the data acquistion thread
	CreateThread(NULL, 8000, WorkerProc, NULL, 0, &ThreadID);
	return TRUE;
}

void SetTitleText(HWND hWnd) {
	TCHAR text[80];
	static bool doneOnce = false;
	if (!doneOnce) {
		swprintf(text, sizeof(text), L"CurveBug   (Ver:%s HW:%d FW:%d) S/N: %d",
			VERSION_STRING, DeviceID[3], DeviceID[2],*(WORD*)&DeviceID[0]);
		SetWindowText(hWnd, text);
	}
}


void DoPaint(HWND hWnd) {
	HDC hdc, Memhdc;
	PAINTSTRUCT ps;
	HBITMAP Membitmap;
	RECT rc;
	long xScale, yScale, width, height, xOffset, floor;
	WORD i, ii;
	static POINT BlackLine[N_POINTS];	// vertices of black polyline
	static POINT RedLine[N_POINTS];		// " " of Red Line

	GetClientRect(hWnd, &rc);			// What are we repainting?

	// figure out the bounds and scaling of what we're drawing
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
	floor = height * 7 / 8 + rc.top;
	xOffset = (rc.left + width - 1);
	xScale = ((DWORD)width << 16) / ADC_MAX;	// eschew floating point
	yScale = (height << 16) / (ADC_MAX - 700);	


	hdc = BeginPaint(hWnd, &ps);	// mystic call that Windows requires
	hdc = GetDC(hWnd);

	// create off-screen display bitmap to paint on
	Memhdc = CreateCompatibleDC(hdc);
	Membitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(Memhdc, Membitmap);

	// Background color for the bitamp
	FillRect(Memhdc, &rc, hBackGround);
	SetBkColor(Memhdc, BACKGROUND_COLOR); // for text drawing

	// grab Mutex so we can insure a coherent data set
	WaitForSingleObject(hMutex, INFINITE);

	// scale and offset the data into poly-line vertices
	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		BlackLine[ii].x = xOffset - ((xScale * DataPoints[1 + i]) >> 16);
		BlackLine[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[1 + i])) >> 16) + floor;
	}

	for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
		RedLine[ii].x = xOffset - ((xScale * DataPoints[2 + i]) >> 16);
		RedLine[ii].y = ((yScale * (DataPoints[0 + i] - DataPoints[2 + i])) >> 16) + floor;
	}
	// we're done with the data points, allow updates
	ReleaseMutex(hMutex);

	// paint the poly-lines
	SelectObject(Memhdc, hBlackPen);
	Polyline(Memhdc, BlackLine, N_POINTS);
	if (!SingleTrace){
		SelectObject(Memhdc, hRedPen);
		Polyline(Memhdc, RedLine, N_POINTS);
	}
	SelectObject(Memhdc, hGreenPen);

	// draw an origin marker
	DWORD xOrigin = xOffset - ((xScale * 2048) >> 16);
	DWORD yOrigin = floor;
	MoveToEx(Memhdc, xOrigin - 15, yOrigin, NULL);
	LineTo(Memhdc, xOrigin + 15, yOrigin);
	MoveToEx(Memhdc, xOrigin, yOrigin-15, NULL);
	LineTo(Memhdc, xOrigin, yOrigin+15);
	
	// if we have a dual display (strong & weak) draw the weak lines
	if (DriveMode == dual) {
		xScale = xScale * 3 / 4;
		yScale = yScale * 3 / 4;
		xOffset -= width / 4;
		floor -= height / 4;

		WaitForSingleObject(hMutex, INFINITE);
		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			BlackLine[ii].x = xOffset - ((xScale * AltData[2 + i]) >> 16);
			BlackLine[ii].y = ((yScale * (AltData[0 + i] - AltData[2 + i])) >> 16) + floor;
		}

		for (ii = i = 0; i < N_POINTS * 3; i += 3, ii++) {
			RedLine[ii].x = xOffset - ((xScale * AltData[1 + i]) >> 16);
			RedLine[ii].y = ((yScale * (AltData[0 + i] - AltData[1 + i])) >> 16) + floor;
		}
		ReleaseMutex(hMutex);

		SelectObject(Memhdc, hPinkPen);
		Polyline(Memhdc, BlackLine, N_POINTS);
		SelectObject(Memhdc, hGrayPen);
		Polyline(Memhdc, RedLine, N_POINTS);
	}

	// draw some text
	TCHAR text[20];
	if (DriveMode == weak) {
		TextOut(Memhdc, 15, 15, L"WEAK", 4);
	}
	_itot(scans, text, 10);
	TextOut(Memhdc, 15, 30, text, _tcsclen(text));

	// Blt the off-screen data onto the display
	BitBlt(hdc, 0, 0, width, height, Memhdc, 0, 0, SRCCOPY);

	// clean up 
	DeleteObject(Membitmap);
	DeleteDC(Memhdc);
	DeleteDC(hdc);
	SetTitleText(hWnd);
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
		wmId = LOWORD(wParam);
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
		Hold = false;
		switch (wParam) {
		case VK_SPACE:
			switch (DriveMode) {
			case strong:
				DriveMode = weak;
				break;
			case weak:
				DriveMode = dual;
				break;
			case dual:
				DriveMode = strong;
			}
			break;
		case VK_F1:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case 'P':
			Hold = true;
			break;
		case 'S':
			SingleTrace = !SingleTrace;
			break;
		}
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

void Damnit(PTCHAR msg) {
	WCHAR errMsg[128];

	if (msg == NULL) {
		msg = errMsg;
		_wcserror_s(errMsg, sizeof(errMsg) / sizeof(*errMsg), GetLastError());
	}
	MessageBox(NULL, msg, L"Bummer!", MB_SYSTEMMODAL);
	exit(1);
}

