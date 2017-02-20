#include <windows.h>
#include <stdlib.h>
#include "proc_info.h"

static void CALLBACK foreground_changed(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	char executable[2048] = {0}, text[2048] = {0}, class[2048] = {0};

	if (get_window_info(new_foreground, class, 2048, text, 2048, executable, 2048) != TRUE)
		return;

	///
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWINEVENTHOOK hook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
		NULL,
		foreground_changed,
		0,
		0,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	);
	if (!hook) abort();

	MSG msg;
	BOOL ret;
	while (0 != (ret = GetMessage(&msg, NULL, 0, 0))) {
		if (ret == -1)	{
			abort();
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UnhookWinEvent(hook);

	return msg.wParam; 
}
/*
PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE);
*/

