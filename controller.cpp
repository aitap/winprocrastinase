#include "win_utils.hpp" // defines _WIN32_WINNT, must be first

#include <fstream>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <memory>
#include <iterator>

#include <windows.h>
#include <shellapi.h>

// NB: it might be a bad idea to leave such objects initialized with nullptr
typedef std::unique_ptr<std::remove_pointer<HWINEVENTHOOK>::type,decltype(&UnhookWinEvent)> u_wineventhook;
typedef std::unique_ptr<std::remove_pointer<HKEY>::type,decltype(&RegCloseKey)> u_key;

// constants
const static float work_per_play = 2.; // how many times more time user should spend working to afford same amount of play time
const static uint32_t alarm_timeout = 60*1000; // how much time to give between the alarm sound and killing the offending app
const char* reg_value_name = "credit_left"; // name of the value used to store remaining time in the registry
const int64_t default_timeout = 60*60*1000; // first hour is free (ms)

// horrible global variables
DWORD prev_event_time = 0; // to be filled in main
int64_t play_credit = 0; // same
enum class user_state_t { neutral, work, play } user_state = user_state_t::neutral; // what the user is doing right now
// set of paths causing credit to increase or decrease; set of window title substrings absolving blacklisted apps
std::unordered_set<std::string> file_whitelist, file_blacklist, title_whitelist;
UINT_PTR alarm_timer = 0; // timer to alarm sound
UINT_PTR kill_timer = 0; // timer to kill of the offender
u_wineventhook title_hook{nullptr,&UnhookWinEvent}; // hook on window title changes
u_key persistent_play_credit{nullptr,&RegCloseKey};

user_state_t decide_window_role(HWND wnd) {
	try {
		std::string path = get_window_process_path(wnd), title = get_window_title(wnd);
		if (file_whitelist.count(path)) return user_state_t::work;
		if (file_blacklist.count(path)) {
			for (const std::string & substr: title_whitelist)
				if (title.find(substr) != std::string::npos) return user_state_t::neutral; // absolved for now
			return user_state_t::play;
		}
	} catch(std::runtime_error & ex) {
		// whatever happened there could only mean that we don't have right for that
	}
	// falling through try or catch, we decide or at least assume neutral result
	return user_state_t::neutral;
}

void CALLBACK kill_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	kill_timer = 0;
	HWND target = GetForegroundWindow();
	try {
		if (decide_window_role(target) == user_state_t::play) // just in case
			kill_window_process(target);
	} catch (std::runtime_error & ex) {
		// a number of things could happen there, most of which mean we don't have enough rights. oh well
	}
}

void CALLBACK alarm_callback(HWND hwnd, UINT message, UINT_PTR timer, DWORD now) {
	KillTimer(hwnd, timer);
	alarm_timer = 0;
	PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE|SND_ASYNC);
	kill_timer = SetTimer(NULL, kill_timer, alarm_timeout, kill_callback);
}

void disarm(void) {
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

void arm(void) {
	alarm_timer = SetTimer(NULL, alarm_timer, play_credit, alarm_callback);
}

void note_window_changes(HWND);
void CALLBACK title_changed
(HWINEVENTHOOK ev_hook, DWORD event, HWND window, LONG object, LONG child, DWORD thread, DWORD time) {
	if (window != GetForegroundWindow()) return; // we're only interested in the foreground window until it changes
	note_window_changes(window);
}

void CALLBACK foreground_changed
(HWINEVENTHOOK ev_hook, DWORD event, HWND wnd, LONG object, LONG child, DWORD thread, DWORD time) {
	DWORD tid, pid;
	tid = GetWindowThreadProcessId(wnd, &pid);
	// watch for changes in the title
	title_hook.reset(SetWinEventHook(
		EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE,
		NULL,
		title_changed,
		pid, tid,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	));
	note_window_changes(wnd);
}

void note_window_changes(HWND wnd) {
	auto time_diff_ms = GetTickCount() - prev_event_time; // to adjust credit
	prev_event_time = GetTickCount(); // and remember what time is it now for the next time

	// account for whatever was happening until window switch
	play_credit += (
		user_state == user_state_t::work ? 1/work_per_play
		: user_state == user_state_t::play ? -1
		: 0
	)*time_diff_ms;
	// with overdrawn credit, set timeout to minimal possible
	if (play_credit < USER_TIMER_MINIMUM) play_credit = USER_TIMER_MINIMUM;
	RegSetValueEx(
		(HKEY)persistent_play_credit.get(),
		reg_value_name,
		0,
		REG_QWORD,
		(const BYTE*)&play_credit,
		sizeof(play_credit)
	);

	user_state = decide_window_role(wnd);

	switch (user_state) {
	case user_state_t::work:
	case user_state_t::neutral:
		disarm();
		break;
	case user_state_t::play:
		arm();
		break;
	}
}

template<typename T> void read_text_into(const char* from, T& to) {
	std::ifstream fh(from, std::ios::in);
	if (!fh) throw std::runtime_error(std::string("Error opening ") + from);
	std::string line;
	while (std::getline(fh, line))
		to.insert(line);
}

int main(int argc, char** argv) try {
	using std::runtime_error;

	u_handle self_mutex {CreateMutex(nullptr,TRUE,"Local\\aitap_WinProcrastinase"),&CloseHandle};
	if (GetLastError() == ERROR_ALREADY_EXISTS) throw runtime_error("Another copy of WinProcrastinase seems to be already running (can't obtain mutex)");
	if (!self_mutex) throw runtime_error("CreateMutex failed");

	read_text_into("whitelist.txt",file_whitelist);
	read_text_into("blacklist.txt",file_blacklist);
	read_text_into("title_exceptions.txt",title_whitelist);

	for (const std::unordered_set<std::string> & set: {file_blacklist, file_whitelist})
		for (const std::string & str: set) {
			std::ifstream test(str);
			if (!test)
				throw runtime_error(std::string("Sanity check failed, file does not seem to exist: ") + str);
		}

	{
		HKEY key;
		if (ERROR_SUCCESS != RegCreateKeyEx(
			HKEY_CURRENT_USER,
			"Software\\aitap\\winprocrastinase",
			0,
			nullptr,
			REG_OPTION_NON_VOLATILE,
			KEY_READ|KEY_WRITE,
			nullptr,
			&key,
			nullptr // lpdwDisposition might be important
		))
			throw runtime_error("RegCreateKeyEx returned error");

		persistent_play_credit.reset(key);
	}
	{
		DWORD play_credit_size = sizeof(play_credit); // oh well
		if (ERROR_SUCCESS != RegGetValue(
			(HKEY)persistent_play_credit.get(),
			nullptr,
			reg_value_name,
			RRF_RT_REG_QWORD,
			nullptr,
			&play_credit,
			&play_credit_size
		))
			play_credit = default_timeout;
	}

	prev_event_time = GetTickCount();
	u_wineventhook foreground_hook{SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
		NULL,
		foreground_changed,
		0,
		0,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	),&UnhookWinEvent};
	if (!foreground_hook) throw runtime_error(std::string("SetWinEventHook error ")+std::to_string(GetLastError()));

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

	if (ERROR_SUCCESS != RegSetValueEx(
		(HKEY)persistent_play_credit.get(),
		reg_value_name,
		0,
		REG_QWORD,
		(const BYTE*)&play_credit,
		sizeof(play_credit)
	))
		throw runtime_error("RegSetValueEx returned error");

	return 0;
} catch (std::runtime_error & e) {
	MessageBox(NULL,e.what(),"Unhandled exception",MB_ICONERROR);
	return 1;
}
