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

BY_HANDLE_FILE_INFORMATION get_file_info(const char* path) {
	handle fh = (
		CreateFile(path, 0, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)
	);
	if (fh == INVALID_HANDLE_VALUE) throw std::runtime_error(std::string("CreateFile returned ")+std::to_string(GetLastError()));
	BY_HANDLE_FILE_INFORMATION ret;
	if (!GetFileInformationByHandle(fh, &ret))
		throw std::runtime_error(std::string("GetFileInformationByHandle returned ")+std::to_string(GetLastError()));
	return ret;
}

bool operator== (const BY_HANDLE_FILE_INFORMATION & a, const BY_HANDLE_FILE_INFORMATION & b) {
	return
		a.dwVolumeSerialNumber == b.dwVolumeSerialNumber
		&& a.nFileIndexLow == b.nFileIndexLow
		&& a.nFileIndexHigh == b.nFileIndexHigh;
}
