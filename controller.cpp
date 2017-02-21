#include <iostream> // DEBUG
using namespace std;
#include <fstream>
#include <set>
#include <stdexcept>
#include <memory>

#include <windows.h>
#include <shellapi.h>

#include "win_utils.hpp"

// constants
//const static float work_per_play = 2; // how many times more time user should spend working to afford same amount of play time
const static float work_per_play = 1; // how many times more time user should spend working to afford same amount of play time
//const static uint32_t alarm_timeout = 60*1000; // time between alarm and projected (credit=0), ms
const static uint32_t alarm_timeout = 3*1000; // time between alarm and projected (credit=0), ms

// horrible global variables
//int64_t play_credit = 3600*1000; // remaining non-work time, ms; user is given 1 hour for free at startup
int64_t play_credit = 10*1000; // remaining non-work time, ms; user is given 1 hour for free at startup
bool good = true; // whether the user is good for now
std::set<std::string> whitelist; // set of paths allowed to be running indefinitely
UINT_PTR alarm_timer = 0; // timer to alarm sound
UINT_PTR kill_timer = 0; // timer to kill of the offender

extern "C" void CALLBACK kill_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	kill_timer = 0;
	/*DEBUG*/ cout << "kill callback" << std::endl;
	kill_window_process(GetForegroundWindow());
}

extern "C" void CALLBACK alarm_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	alarm_timer = 0;
	/*DEBUG*/ cout << "alarm callback" << std::endl;
	PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE|SND_ASYNC);
	kill_timer = SetTimer(NULL, kill_timer, alarm_timeout, kill_callback);
}

extern "C" void CALLBACK foreground_changed
(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	static auto prev_event = GetTickCount(); // remember time of previous event
	auto time_diff_ms = GetTickCount() - prev_event; // adjust credit using (now-previous)
	prev_event = GetTickCount(); // and remember what time is it now for the next time

	/*DEBUG*/cout << "window callback: " << time_diff_ms << " ms elapsed; credit: " << play_credit << " -> ";

	// account for whatever was happening until window switch
	play_credit += (good ? 1/work_per_play : -1)*time_diff_ms;
	// with overdrawn credit, set timeout to minimal possible
	if (play_credit < USER_TIMER_MINIMUM) play_credit = USER_TIMER_MINIMUM;

	/*DEBUG*/cout << play_credit << std::endl;

	// prepare the bookkeeping for updating next time
	try {
		std::string path = get_window_process_path(new_foreground);
		/*DEBUG*/cout << "path=" << path;
		good = whitelist.count(path);
	} catch (std::runtime_error & ex) {
		// whatever happened, let's presume innocence
		good = true;
	}
	/*DEBUG*/cout << " good=" << good << std::endl;
	if (good) { // whitelisted
		if (kill_timer) {
			KillTimer(NULL, kill_timer);
			kill_timer = 0;
		}
		if (alarm_timer) {
			KillTimer(NULL, alarm_timer);
			alarm_timer = 0;
		}
		PlaySound(NULL,0,0);
	} else { // non-whitelisted
		alarm_timer = SetTimer(NULL, alarm_timer, play_credit, alarm_callback);
	}
}

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

	/*DEBUG*/for (const std::string & path: whitelist) cout << "Whitelisted: " << path << std::endl;

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
			throw runtime_error("GetMessage returned -1");
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}
