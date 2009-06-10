#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aochat.h>


#define WM_ASYNCSELECT	(WM_USER+123)


HWND hMainWnd;
aocConnection *aoc;
int disconnected;

void aocWindowsPoll()
{
	aocEvent *event;
	int flags;
	long lEvent = 0;


	while( (event = aocEventGet()) )
	{
		switch(event->type)
		{
		case AOC_EVENT_CONNECT:
			MessageBox(0, "Connected.", "libaochat", 0);
			break;
		case AOC_EVENT_CONNFAIL:
			MessageBox(0, "Connection failed.", "libaochat", 0);
			disconnected = 1;
			break;
		case AOC_EVENT_DISCONNECT:
			MessageBox(0, "Disconnected.", "libaochat", 0);
			disconnected = 1;
			break;
		case AOC_EVENT_MESSAGE:
			break;
		case AOC_EVENT_TIMER:
			break;
		}
		aocEventDestroy(event);
	}

	if(disconnected)
		return;

	flags = aocPollQuery(aoc);

	if(flags & AOC_POLL_READ)
		lEvent |= FD_READ | FD_CLOSE;
	if(flags & AOC_POLL_WRITE)
		lEvent |= FD_WRITE | FD_CONNECT;

	WSAAsyncSelect(aoc->socket, hMainWnd, WM_ASYNCSELECT, lEvent);
}


LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_ASYNCSELECT:
		if(!disconnected && wParam == aoc->socket)
		{
			if(lParam & FD_READ ||lParam & FD_CLOSE)
				aocPollCanRead(aoc);
			if(lParam & FD_WRITE || lParam & FD_CONNECT)
				aocPollCanWrite(aoc);
			aocWindowsPoll();
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}


int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	struct WSAData wd;
	WNDCLASS cls;
	MSG msg;

	cls.style = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc = &MainWindowProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = hInstance;
	cls.hIcon = NULL;
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = GetStockObject(GRAY_BRUSH);
	cls.lpszMenuName = NULL;
	cls.lpszClassName = "WSAAsyncSelect_Test_libaochat";

	RegisterClass(&cls);

	hMainWnd = CreateWindowEx(WS_EX_APPWINDOW, "WSAAsyncSelect_Test_libaochat",
			"WSAAsyncSelect Test (libaochat)", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			0, 0, 256, 256, NULL, NULL, hInstance, NULL);

	WSAStartup(WINSOCK_VERSION, &wd);
	aoc = aocInit(NULL);
	disconnected = 0;
	aocConnect(aoc, aocMakeAddr(AOC_SERVER_RK2, 7012));
	aocWindowsPoll();

	while(GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
