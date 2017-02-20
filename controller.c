#include <windows.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "proc_info.h"

// horrible global variables
static uint32_t credit = 3600*1000; // remaining non-work time, ms
static bool good = true; // whether the user is good for now
float work_to_play = 2; // how many times more time user should spend working to afford same amount of play time

static void CALLBACK foreground_changed(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	char executable[2048] = {0}, text[2048] = {0}, class[2048] = {0};

	if (get_window_info(new_foreground, class, 2048, text, 2048, executable, 2048) != TRUE)
		return; // FIXME: actually, assume it's whitelisted

	//recalculate the "good time" credit
	//if whitelisted: stop any kill timers & sounds, set good=true
	//else: set good=false, set timer for (remaining-5min) to sound alarm
}

// alarm sound callback: remember GetForegroundWindow() and set 5m timer to kill it

// kill callback: check non-whitelistedness of GetForegroundWindow() and EndTask it

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

