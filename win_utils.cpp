#include "win_utils.hpp"
#include <processthreadsapi.h>

std::string get_window_process_path(HWND window) {
	using std::runtime_error;

	char path[2048];

	DWORD thread_id = GetWindowThreadProcessId(window,NULL);
	u_handle thread{OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id),&CloseHandle};
	if (!thread) throw runtime_error(std::string("OpenThread returned ")+std::to_string(GetLastError()));

	DWORD pid = GetProcessIdOfThread(thread.get());
	if (!pid) throw runtime_error(std::string("GetProcessIdOfThread returned ")+std::to_string(GetLastError()));

	u_handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid),&CloseHandle};
	if (!process) throw runtime_error(std::string("OpenProcess returned ")+std::to_string(GetLastError()));

	DWORD size = 2048;
	if (!QueryFullProcessImageName(process.get(), 0/*win32 format*/, path, &size))
		throw runtime_error(std::string("QueryFullProcessImageName returned ")+std::to_string(GetLastError()));

	return std::string(path);
}


std::string get_window_title(HWND window) {
	using std::runtime_error;

	char title[2048];

	if (!GetWindowText(window, title, 2048))
		throw runtime_error(std::string("GetWindowText returned ")+std::to_string(GetLastError()));

	return std::string(title);
}

void kill_window_process(HWND window) {
	using std::runtime_error;
	DWORD thread_id = GetWindowThreadProcessId(window,NULL);
	u_handle thread{OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id),&CloseHandle};
	if (!thread) throw runtime_error(std::string("OpenThread returned ")+std::to_string(GetLastError()));

	DWORD pid = GetProcessIdOfThread(thread.get());
	if (!pid) throw runtime_error(std::string("GetProcessIdOfThread returned ")+std::to_string(GetLastError()));

	u_handle process{OpenProcess(PROCESS_TERMINATE, FALSE, pid),&CloseHandle};
	if (!process) throw runtime_error(std::string("OpenProcess returned ")+std::to_string(GetLastError()));

	if (!TerminateProcess(process.get(), 1)) throw runtime_error(std::string("TerminateProcess returned ")+std::to_string(GetLastError()));
}
