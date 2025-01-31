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
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
bool dualDisplay;
bool Painting = FALSE;

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
	hPinkPen = CreatePen(BS_SOLID, 1, RGB(255,200,200));
	hGrayPen = CreatePen(BS_SOLID, 1, RGB(200,200,200));


   ShowWindow(hWnd, SW_MAXIMIZE);
   UpdateWindow(hWnd);
   CreateThread(NULL,8000, WorkerProc, NULL, 0, &ThreadID);
   return TRUE;
}

#define OFFSET 300

void DoPaint(HWND hWnd){
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rc;
	long xScale, yScale, width;
	WORD x, y, i, floor;
	Painting = TRUE;
	GetClientRect(hWnd, &rc);

	hdc = BeginPaint(hWnd, &ps);
	width = rc.right - rc.left;
	floor = (rc.bottom - rc.top) * 7/ 8 + rc.top;
	xScale = ((DWORD)width << 16) / ADC_MAX;
    yScale = ((rc.bottom - rc.top) << 16) / (ADC_MAX - 700);

	SelectObject(hdc, hBlackPen);
	x = (WORD)((xScale * DataPoints[1]) >> 16);
	x = (rc.left + width-1) - x;
	y = (WORD)((yScale * (DataPoints[0] - DataPoints[1])) >> 16);
	y += floor; // offset??
	MoveToEx(hdc, x, y,0);
    for (i = 3; i < N_POINTS*3; i += 3) {
		x = (WORD)((xScale * DataPoints[1+i]) >> 16);
		x = (rc.left + width - 1) - x;
		y = (WORD)((yScale * (DataPoints[0+i] - DataPoints[1+i])) >> 16);
		y += floor; // offset??

		LineTo(hdc, x, y );
	}

	SelectObject(hdc, hRedPen);
	x = (WORD)((xScale * DataPoints[2]) >> 16);
	x = (rc.left + width - 1) - x;
	y = (WORD)((yScale * (DataPoints[0] - DataPoints[2])) >> 16);
	y += floor; // offset??
	MoveToEx(hdc, x, y, 0);
	for (i = 3; i < N_POINTS * 3; i += 3) {
		x = (WORD)((xScale * DataPoints[2 + i]) >> 16);
		x = (rc.left + width - 1) - x;
		y = (WORD)((yScale * (DataPoints[0 + i] - DataPoints[2 + i])) >> 16);
		y += floor; // offset??

		LineTo(hdc, x, y);

	}
	Painting = FALSE;
	Sleep(20);
/*
	if (dualDisplay){
		xScale /= 2;
		yScale /= 2;
		Width /= 2;
		int VertOffset = (rc.bottom + rc.top)/2;

		SelectObject(hdc, hPinkPen);
		x = rc.left + (int)(xScale * AltData[2]) + (Width);
		y = VertOffset + (int)(yScale * (AltData[1] - AltData[2]));
		MoveToEx(hdc, x, y,0);
		for (i = 3; i < N_POINTS * 3; i += 3) {
			x = rc.left + (int)(xScale * AltData[i+2]) + (Width);
			y = VertOffset + (int)(yScale * (AltData[i+1] - AltData[i+2]));

			LineTo(hdc, x, y );
		}
		SelectObject(hdc, hGrayPen);
		x = rc.left + (int)(xScale * AltData[0]) + (Width);
		y = VertOffset + (int)(yScale * (AltData[1] - AltData[0]));
		MoveToEx(hdc, x, y,0);
		for (i = 3; i < N_POINTS * 3; i += 3) {
			x = rc.left + (int)(xScale * AltData[i]) + (Width);
			y = VertOffset + (int)(yScale * (AltData[i+1] - AltData[i]));

			LineTo(hdc, x, y );
		}
	}
	*/

  //  TextOut(hdc, 20, 20, L"Hello, Windows!", 15); 

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

void Damnit() {
	LPTSTR txt = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&txt,
		0,
		NULL);
	MessageBox(NULL, txt, L"Oops!", MB_SYSTEMMODAL);
	LocalFree(txt);
	exit(1);
}


#if 0
/*
this code goes in your window procedure in the WM_PAINT message handler
NOTE: you may need to set the window background not to draw itself
before you register the window class:
[NAME_OF_YOUR_WNDCLASSEX].hbrBackground	= NULL;
*/

RECT Client_Rect;
GetClientRect(hWnd, &Client_Rect);
int win_width = Client_Rect.right - Client_Rect.left;
int win_height = Client_Rect.bottom + Client_Rect.left;
PAINTSTRUCT ps;
HDC Memhdc;
HDC hdc;
HBITMAP Membitmap;
hdc = BeginPaint(hWnd, &ps);
Memhdc = CreateCompatibleDC(hdc);
Membitmap = CreateCompatibleBitmap(hdc, win_width, win_height);
SelectObject(Memhdc, Membitmap);
//drawing code goes in here
BitBlt(hdc, 0, 0, win_width, win_height, Memhdc, 0, 0, SRCCOPY);
DeleteObject(Membitmap);
DeleteDC(Memhdc);
DeleteDC(hdc);
EndPaint(hWnd, &ps);

/*
Basically this is whare you are doing:
1  Make a copy of the device context that is the same format as the
default one that is showing (the device context is the object you draw to)
2  Copy what is in the on screen dc to your copied dc
3  Draw on the copy
4  Put what is in the copy back onto the original dc which gets displayed
*/
#endif