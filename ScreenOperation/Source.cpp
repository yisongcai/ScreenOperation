#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>

#define FREE(x)							{if(x != NULL ){free(x);x=NULL;}}
#define RELEASE(x)						{if(x != NULL ){delete x;x=NULL;}}
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { closesocket(x);x=INVALID_SOCKET;}}

DWORD EasyWrite(LPCTSTR lpszFilePath, LPCVOID lpBuffer, DWORD dwBuffer)
{
	DWORD dwNumberOfBytesWrite = 0;
	HANDLE hFile = NULL;
	if (INVALID_HANDLE_VALUE != (hFile = CreateFile(lpszFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, NULL, NULL)))
	{
		if (HFILE_ERROR != SetFilePointer(hFile, 0, 0, FILE_END))
		{
			WriteFile(hFile, lpBuffer, dwBuffer, &dwNumberOfBytesWrite, 0);
		}
		RELEASE_HANDLE(hFile);
	}
	return dwNumberOfBytesWrite;
}

int SaveBitmapToFile(HBITMAP hBitmap, LPCTSTR lpFileName)
{
	int nRetValue = 0;
	HDC hDC = NULL;
	int iBits = 0;
	WORD wBitCount = 0;
	DWORD dwPaletteSize = 0;
	DWORD dwBmBitsSize = 0;
	DWORD dwDIBSize = 0;
	DWORD dwWritten = 0;
	BITMAP Bitmap = { 0 };
	BITMAPFILEHEADER bmfHdr = { 0 };
	BITMAPINFOHEADER bi = { 0 };
	LPBITMAPINFOHEADER lpbi = { 0 };
	HANDLE fh = NULL, hDib = NULL;
	HPALETTE hPal = NULL, hOldPal = NULL;

	if (hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL)) {
		iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
		DeleteDC(hDC);
		hDC = NULL;

		wBitCount = 24;

		if (iBits <= 1)
			wBitCount = 1;
		else if (iBits <= 4)
			wBitCount = 4;
		else if (iBits <= 8)
			wBitCount = 8;
		else if (iBits <= 24)
			wBitCount = 24;

		if (wBitCount <= 8)
			dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);

		GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = Bitmap.bmWidth;
		bi.biHeight = Bitmap.bmHeight;
		bi.biPlanes = 1;
		bi.biBitCount = wBitCount;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

		hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
		lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
		*lpbi = bi;

		if (hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE)) {
			if (hDC = GetDC(NULL)) {
				hOldPal = SelectPalette(hDC, hPal, FALSE);
				RealizePalette(hDC);
			}
		}

		GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

		if (hOldPal) {
			SelectPalette(hDC, hOldPal, TRUE);
		}

		if (hDC) {
			RealizePalette(hDC);
			ReleaseDC(NULL, hDC);
			hDC = NULL;
		}

		bmfHdr.bfType = 0x4D42;
		dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
		bmfHdr.bfSize = dwDIBSize;
		bmfHdr.bfReserved1 = 0;
		bmfHdr.bfReserved2 = 0;
		bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

		if (EasyWrite(lpFileName, &bmfHdr, sizeof(BITMAPFILEHEADER))) {
			if (EasyWrite(lpFileName, lpbi, dwDIBSize)) {
				nRetValue = 1;
			}
		}

		if (hDib) {
			GlobalUnlock(hDib);
			GlobalFree(hDib);
		}
	}

	return nRetValue;
}

int ScreenCapture(LPCTSTR lpFileName)
{
	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, nScreenWidth, nScreenHeight);
	SelectObject(hCaptureDC, hCaptureBitmap);
	BitBlt(hCaptureDC, 0, 0, nScreenWidth, nScreenHeight, hDesktopDC, 0, 0, SRCCOPY);
	SaveBitmapToFile(hCaptureBitmap, lpFileName);
	ReleaseDC(hDesktopWnd, hDesktopDC);
	DeleteDC(hCaptureDC);
	return 0;
}

typedef struct _MsgParam
{
	TCHAR szWindowsHandle[MAX_PATH];
	TCHAR szChildHandle[MAX_PATH];

	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;

	_MsgParam() {
		uMsg = WM_NULL;
		wParam = 0;
		lParam = 0;

		ZeroMemory(szWindowsHandle, sizeof(szWindowsHandle));
		ZeroMemory(szChildHandle, sizeof(szChildHandle));
	}
}MsgParam, *PMsgParam;

BOOL CALLBACK SendKeyProc(HWND hwnd, LPARAM lParam)
{
	PMsgParam pMsg = (PMsgParam)lParam;

	TCHAR szCurrentText[MAX_PATH * 100] = { 0 };
	TCHAR szCurrentHandle[MAX_PATH] = { 0 };

	_stprintf_s(szCurrentHandle, _T("%d"), (UINT)hwnd);
	GetWindowText(hwnd, szCurrentText, sizeof(szCurrentText));

	if (lstrlen(pMsg->szWindowsHandle) != 0 && lstrlen(pMsg->szChildHandle) == 0 && pMsg->uMsg != WM_NULL) {
		PostMessage((HWND)_ttol(pMsg->szWindowsHandle), pMsg->uMsg, pMsg->wParam, pMsg->lParam);
	}

	// WM_LBUTTONDOWN 0x0201 513
	// WM_LBUTTONUP 0x202 514
	// MK_LBUTTON 0x1 
	
	if (lstrlen(pMsg->szWindowsHandle) != 0 && lstrcmpi(szCurrentHandle, pMsg->szChildHandle) == 0 && pMsg->uMsg != WM_NULL) {
		PostMessage((HWND)_ttol(pMsg->szChildHandle), pMsg->uMsg, pMsg->wParam, pMsg->lParam);

		if (pMsg->uMsg == WM_LBUTTONDOWN) {
			PostMessage((HWND)_ttol(pMsg->szChildHandle), WM_LBUTTONUP, pMsg->wParam, pMsg->lParam);
		}
	}



	std::wcout << _T("HWND:") << szCurrentHandle << _T(" PARENT:") << (UINT)GetParent(hwnd) << _T(" TEXT:") << szCurrentText << std::endl;
	return TRUE;
}

BOOL SendKeyToWin(PMsgParam pMsgParam)
{
	FILE *fp = NULL;
	DeleteFile(_T("StdOut.log"));
	setlocale(LC_ALL, "chs");
	freopen_s(&fp, "StdOut.log", "a", stdout);
	if (lstrlen(pMsgParam->szWindowsHandle) == 0) {
		return EnumWindows(SendKeyProc, (LPARAM)pMsgParam);
	}
	else {
		HWND hwnd = (HWND)_ttol(pMsgParam->szWindowsHandle);
		return EnumChildWindows(hwnd, SendKeyProc, (LPARAM)pMsgParam);
	}
}

int Help()
{
	TCHAR szLogOut[MAX_PATH] = { 0 };
	lstrcat(szLogOut, _T("PARENT ´°¿Ú¾ä±ú\n"));
	lstrcat(szLogOut, _T("CHILD ´°¿Ú¾ä±ú\n"));
	lstrcat(szLogOut, _T("UMSG 16\n"));
	lstrcat(szLogOut, _T("WPARAM 0\n"));
	lstrcat(szLogOut, _T("LPARAM 0\n"));
	lstrcat(szLogOut, _T("SCREEN/MESSAGE\n"));
	EasyWrite(_T("help.txt"), szLogOut, lstrlen(szLogOut) * sizeof(TCHAR));
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//SetErrorMode(SEM_NOGPFAULTERRORBOX);

	int nArgc = 0;
	TCHAR **szArgv = CommandLineToArgvW(GetCommandLineW(), &nArgc);

	if (nArgc == 1) {
		return Help();
	}

	MsgParam msg;
	
	for (int i = 1;i < nArgc;i++) {
		if (lstrcmpi(szArgv[i], _T("UMSG")) == 0) {
			msg.uMsg = _ttoi(szArgv[++i]);
		}
		else if (lstrcmpi(szArgv[i], _T("LPARAM")) == 0) {
			msg.lParam = _ttoi(szArgv[++i]);
		}
		else if (lstrcmpi(szArgv[i], _T("WPARAM")) == 0) {
			msg.wParam = _ttoi(szArgv[++i]);
		}
		else if (lstrcmpi(szArgv[i], _T("PARENT")) == 0) {
			lstrcpy(msg.szWindowsHandle, szArgv[++i]);
		}
		else if (lstrcmpi(szArgv[i], _T("CHILD")) == 0) {
			lstrcpy(msg.szChildHandle, szArgv[++i]);
		}
		else if (lstrcmpi(szArgv[i], _T("SCREEN")) == 0) {
			ScreenCapture(_T("tmp.bmp"));
		}
		else if (lstrcmpi(szArgv[i], _T("MESSAGE")) == 0) {
			SendKeyToWin(&msg);
		}
	}

	LocalFree(szArgv);
	return 0;
}