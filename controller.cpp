#include <fstream>
#include <set>
#include <stdexcept>
#include <memory>

#include <windows.h>
#include <shellapi.h>

#include "win_utils.hpp"

// constants
const static float work_per_play = 2; // how many times more time user should spend working to afford same amount of play time
const static uint32_t alarm_timeout = 60*1000; // time between alarm and projected (credit=0), ms

// horrible global variables
static uint32_t credit = 3600*1000; // remaining non-work time, ms; user is given 1 hour for free at startup
static bool good = true; // whether the user is good for now
static float work_per_play = 2; // how many times more time user should spend working to afford same amount of play time
std::set<std::string> whitelist;

extern "C" void CALLBACK foreground_changed(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	try {
		std::string path = get_window_process_path(new_foreground);
	} catch (std::runtime_error & ex) {
		// TODO: assume the app is whitelisted
		// shit happens, but we have to keep running
		return;
	}
	//TODO:
	//recalculate the "good time" credit
	//if whitelisted: stop any kill timers & sounds, set good=true
	//else: set good=false, set timer for (remaining-5min) to sound alarm
}

// TODO: alarm sound callback: remember GetForegroundWindow() and set 5m timer to kill it

// TODO: kill callback: check non-whitelistedness of GetForegroundWindow() and EndTask it

// TODO: bool is_whitelisted(HWND)

int main(int argc, char** argv) {
	using std::runtime_error;
	if (argc != 2) {
		MessageBox(NULL,"Please pass path to whitelist.txt as the only command line argument.",NULL,0);
		return 1;
	}

	{
		std::ifstream fh(argv[1],std::ios::in);
		if (!fh) throw runtime_error(std::string("Error opening ")+argv[1]);
		for (std::string path; std::getline(fh, path);) {
			whitelist.insert(path);
		}
	}

	std::unique_ptr<std::remove_pointer<HWINEVENTHOOK>::type,decltype(&UnhookWinEvent)> hook{SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
		NULL,
		foreground_changed,
		0,
		0,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	),&UnhookWinEvent};
	if (!hook) throw runtime_error(std::string("SetWinEventHook error "+GetLastError()));

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

	return 0;
}
/*
PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE);
*/

