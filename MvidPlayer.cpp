#include "Midi.h"

#ifdef BUILD_PLAYER

#include <cstdio>
#include <windows.h>
#pragma comment(lib, "winmm.lib")

HDC hdc;
HMIDIOUT hmo;
DWORD dwRead;

constexpr UINT32 FrmBufCount = 120;
constexpr UINT32 VidPixelCount = VidHeight * VidWidth, VidMemLen = VidPixelCount * sizeof(UINT32);
constexpr UINT32 FrmBufPxCnt = FrmBufCount * VidPixelCount, FrmBufLen = FrmBufPxCnt * sizeof(UINT32);
volatile PUINT32 FrmBufA, FrmBufB;
volatile UINT32 iUsedCnt = 0, iAvailCnt = 0;
volatile BOOL StillLeft = TRUE;
DWORD WINAPI ThreadVideoPlayer(LPVOID mfx);
DWORD WINAPI ThreadFrameLoader(LPVOID phfile);
BOOL OpenWindow(HWND& hWnd, UINT32 vw, UINT32 vh);

int main(int arg, char ** argv) {
	HWND hWnd;
	if (!OpenWindow(hWnd, VidWidth, VidHeight)) return 1;
	hdc = GetDC(hWnd);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	RECT crect, wrect;
	GetClientRect(hWnd, &crect);
	GetWindowRect(hWnd, &wrect);
	MoveWindow(hWnd,
		wrect.left,
		wrect.top,
		wrect.right - wrect.left + VidWidth - crect.right,
		wrect.bottom - wrect.top + VidHeight - crect.bottom, FALSE
	);
	HANDLE hFile = CreateFile(VID_FILENAME, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	MidiFile *mftop = MidiFile::OpenFile(MID_FILENAME);
	FrmBufA = new UINT32[FrmBufPxCnt];
	FrmBufB = new UINT32[FrmBufPxCnt];
	ReadFile(hFile, FrmBufA, FrmBufLen, &dwRead, NULL); iAvailCnt = FrmBufCount;
	ReadFile(hFile, FrmBufB, FrmBufLen, &dwRead, NULL);
	midiOutOpen(&hmo, 1, CALLBACK_NULL, NULL, 0);
	HANDLE hThrVidPly = CreateThread(NULL, 0, ThreadVideoPlayer, mftop, 0, NULL);
	HANDLE hThrLdFile = CreateThread(NULL, 0, ThreadFrameLoader, &hFile, 0, NULL);
	SetThreadIdealProcessor(hThrVidPly, 1);
	SetThreadIdealProcessor(hThrLdFile, 2);
	CloseHandle(hThrVidPly);
	CloseHandle(hThrLdFile);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
		DispatchMessage(&msg);
	midiOutClose(hmo);
	delete FrmBufA;
	delete FrmBufB;
	delete mftop;
	CloseHandle(hFile);
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
BOOL OpenWindow(HWND& hWnd, UINT32 vw, UINT32 vh) {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MidiVis";
	if (!RegisterClass(&wc))
		return FALSE;
	hWnd = CreateWindow("MidiVis",
		"MidiVis",
		WS_BORDER | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		vw, vh,
		NULL, NULL, wc.hInstance, NULL
	);
	if (hWnd == NULL)
		return FALSE;
	return TRUE;
}
DWORD WINAPI ThreadFrameLoader(LPVOID phfile) {
	HANDLE hFile = *(HANDLE*)phfile;
	PUINT32 pTmp;
	while (TRUE) {
		while (iAvailCnt - iUsedCnt > 0) { Sleep(1); }
		pTmp = FrmBufA;
		FrmBufA = FrmBufB;
		FrmBufB = pTmp;
		iAvailCnt += dwRead / VidMemLen;
		ReadFile(hFile, FrmBufB, FrmBufLen, &dwRead, NULL);
		if (dwRead == 0)
			break;
	}
	StillLeft = FALSE;
	return 0;
}

DWORD WINAPI ThreadVideoPlayer(LPVOID mfx) {
	MidiFile *mf = ((MidiFile*)mfx)->Copy();
	MidiTrk *trks = mf->trks, *trk;
	BYTE ret, ch, key;
	UINT16 ntrk = mf->ntrk, divs = mf->divs, i;
	UINT32 scrh = (UINT32)(ScrHeight * divs),
		mindt, msg, scrt = 0, mpqn = 500000;
	PUINT32 vidMem;
	for (i = 0; i < ntrk; i++)
		if (trks[i].not_end)
			trks[i].dt = trks[i].ReadVL() + scrh;
	LARGE_INTEGER
		frequency, cnt_before, cnt_current, cnt_duration;

	system("pause");

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&cnt_before);
	UINT64
		frm_minute = 0,
		frm_second = 0,
		frm_frmcnt = 0;
	UINT64
		time_current = 0, time_next = 0,
		time_nextevt = 0,
		time_nextfrm = 0,
		sleep_micros;
	BITMAPINFO bi;
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = VidWidth;
	bi.bmiHeader.biHeight = VidHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	while (mf->NotAllEnd()) {
		if (time_current == time_nextevt) {
			for (i = 0, mindt = -1, trk = trks; i < ntrk; i++, trk++) {
				while (trk->not_end&&trk->dt == 0) {
					ret = trk->ReadEvent_Dn(ch, key, msg, mpqn);
					if (ret == 0x80 || ret == 0x90)
						midiOutShortMsg(hmo, msg);
					if (trk->not_end)
						trk->ReadDelta();
				}
				if (trk->not_end&&trk->dt < mindt)
					mindt = trk->dt;
			}
			if (mindt != -1) {
				for (i = 0; i < ntrk; i++)
					trks[i].dt -= mindt;
				time_nextevt += (UINT64)mindt * mpqn / divs;
			}
		}
		if (time_current == time_nextfrm) {
			constexpr UINT64 FrameInterval = 1000000 / Framerate;
			time_nextfrm += FrameInterval;
			//
			while (iUsedCnt >= iAvailCnt) {
				if (StillLeft) Sleep(1);
				else break;
			}
			if (StillLeft) {
				vidMem = FrmBufA + (iUsedCnt % FrmBufCount) * VidPixelCount;
				StretchDIBits(hdc, 0, 0, VidWidth, VidHeight, 0, 0, VidWidth, VidHeight, vidMem, &bi, DIB_RGB_COLORS, SRCCOPY);
				iUsedCnt++;
				//
				printf("\rTime: %02llu:%02llu.%02lluf nAvailFrm = %d", frm_minute, frm_second, frm_frmcnt, iAvailCnt - iUsedCnt);
				frm_frmcnt++;
				if (frm_frmcnt >= Framerate) {
					frm_frmcnt = 0;
					frm_second++;
					if (frm_second >= 60) {
						frm_second = 0;
						frm_minute++;
					}
				}
			}
		}
		//
		time_next = time_nextevt;
		if (time_next > time_nextfrm) time_next = time_nextfrm;
		sleep_micros = time_next - time_current;
		/*if (sleep_micros > 1000)
			Sleep((UINT32)sleep_micros / 1000);*/
		cnt_duration.QuadPart = frequency.QuadPart * sleep_micros / 1000000;
		do QueryPerformanceCounter(&cnt_current);
		while (cnt_current.QuadPart - cnt_before.QuadPart < cnt_duration.QuadPart);
		time_current = time_next;
		QueryPerformanceCounter(&cnt_before);
	}
	delete mf;
	PostQuitMessage(0);
	printf("\nEND\n");
	return 0;
}

#endif