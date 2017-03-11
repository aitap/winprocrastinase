#include <fstream>
#include <set>
#include <vector>
#include <stdexcept>
#include <memory>

#include <windows.h>
#include <shellapi.h>

#include "win_utils.hpp"

typedef std::unique_ptr<std::remove_pointer<HWINEVENTHOOK>::type,decltype(&UnhookWinEvent)> raii_hook;

// constants
const static float work_per_play = 2; // how many times more time user should spend working to afford same amount of play time
const static uint32_t alarm_timeout = 60*1000; // time between alarm and projected (credit=0), ms

// horrible global variables
int64_t play_credit = 3600*1000; // remaining non-work time, ms; user is given 1 hour for free at startup
bool good = true; // whether the user is good for now
std::set<std::string> file_whitelist; // set of paths allowed to be running indefinitely
std::vector<std::string> title_whitelist; // set of window titles allowed to be running indefinitely
UINT_PTR alarm_timer = 0; // timer to alarm sound
UINT_PTR kill_timer = 0; // timer to kill of the offender
raii_hook title_hook{nullptr,&UnhookWinEvent}; // hook on window title changes

bool is_title_whitelisted(std::string& title) {
	for (std::string & substr: title_whitelist) {
		if (title.find(substr) != std::string::npos) return true;
	}
	return false;
}

extern "C" void CALLBACK kill_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	kill_timer = 0;
	HWND target = GetForegroundWindow();
	try {
		std::string title = get_window_title(target);
		if (
			!file_whitelist.count(get_window_process_path(target))
			&& !is_title_whitelisted(title)
		) // one last chance
			kill_window_process(target);
	} catch (std::runtime_error & ex) {
		// oh well, what could we do
	}
}

extern "C" void CALLBACK alarm_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	alarm_timer = 0;
	PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE|SND_ASYNC);
	kill_timer = SetTimer(NULL, kill_timer, alarm_timeout, kill_callback);
}

void set_good(void) {
	good = true;
	if (kill_timer) {
		KillTimer(NULL, kill_timer);
		kill_timer = 0;
	}
	if (alarm_timer) {
		KillTimer(NULL, alarm_timer);
		alarm_timer = 0;
	}
	PlaySound(NULL,0,0);
}

void set_bad(void) {
	good = false;
	alarm_timer = SetTimer(NULL, alarm_timer, play_credit, alarm_callback);
}

extern "C" void CALLBACK title_changed
(HWINEVENTHOOK ev_hook, DWORD event, HWND window, LONG object, LONG child, DWORD thread, DWORD time) {
	if (window != GetForegroundWindow()) return; // we're only interested in the foreground window, until it changes
	std::string title = get_window_title(window);

	if (is_title_whitelisted(title))
		set_good();
	else
		set_bad();
}

extern "C" void CALLBACK foreground_changed
(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	if (title_hook) // the window's changed, no reason to keep listening to title changes
		title_hook.reset(nullptr);

	static auto prev_event = GetTickCount(); // remember time of previous event
	auto time_diff_ms = GetTickCount() - prev_event; // adjust credit using (now-previous)
	prev_event = GetTickCount(); // and remember what time is it now for the next time

	// account for whatever was happening until window switch
	play_credit += (good ? 1/work_per_play : -1)*time_diff_ms;
	// with overdrawn credit, set timeout to minimal possible
	if (play_credit < USER_TIMER_MINIMUM) play_credit = USER_TIMER_MINIMUM;

	// prepare the bookkeeping for updating next time
	try {
		std::string path = get_window_process_path(new_foreground);
		good = file_whitelist.count(path);
	} catch (std::runtime_error & ex) {
		// whatever happened, let's presume innocence
		good = true;
	}

	if (good) { // path is whitelisted
		set_good();
	} else { // path is non-whitelisted
		try {
			std::string title = get_window_title(new_foreground);
			if (is_title_whitelisted(title)) { // but the title is
				set_good();

				DWORD tid, pid;
				tid = GetWindowThreadProcessId(new_foreground, &pid);
				title_hook.reset(SetWinEventHook( // but watch for changes in the title
					EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE,
					NULL,
					title_changed,
					pid, tid,
					WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
				));
			} else {
				set_bad();
			}
		} catch (std::runtime_error & ex) { // still presume innocence
			set_good();
		}
	}
}

int main(int argc, char** argv) {
	using std::runtime_error;
	if (argc != 3) {
		MessageBox(NULL,"Please pass path to file_whitelist.txt and title_whitelist.txt as the only command line arguments.",NULL,0);
		return 1;
	}

	{
		std::ifstream fh(argv[1],std::ios::in);
		if (!fh) throw runtime_error(std::string("Error opening ")+argv[1]);
		for (std::string path; std::getline(fh, path);) {
			file_whitelist.insert(path);
		}
	}

	{
		std::ifstream fh(argv[2],std::ios::in);
		if (!fh) throw runtime_error(std::string("Error opening ")+argv[2]);
		for (std::string path; std::getline(fh, path);) {
			title_whitelist.push_back(path);
		}
	}

	raii_hook foreground_hook{SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
		NULL,
		foreground_changed,
		0,
		0,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	),&UnhookWinEvent};
	if (!foreground_hook) throw runtime_error(std::string("SetWinEventHook error "+GetLastError()));

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
