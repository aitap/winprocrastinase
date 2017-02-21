#define _WIN32_WINNT 0x601

#include <memory>

#include <windows.h>
#include <processthreadsapi.h>

#include "proc_info.hpp"

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,decltype(&CloseHandle)> raii_handle;

std::string get_window_process_path(HWND window) {
	using std::runtime_error;

	std::string path;

	DWORD thread_id = GetWindowThreadProcessId(window,NULL);
	raii_handle thread{OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id),&CloseHandle};
	if (!thread) throw runtime_error("OpenThread returned "+GetLastError());

	DWORD pid = GetProcessIdOfThread(thread.get());
	if (!pid) throw runtime_error("GetProcessIdOfThread returned "+GetLastError());

	raii_handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid),&CloseHandle};
	if (!process) throw runtime_error("OpenProcess returned "+GetLastError());

	DWORD size_dword = 2048;
	path.reserve(size_dword);
	if (!QueryFullProcessImageName(process.get(), 0/*win32 format*/, &path[0], &size_dword))
		throw runtime_error("QueryFullProcessImageName returned "+GetLastError());
	path.shrink_to_fit();

	return path;
}