#define _WIN32_WINNT 0x601
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <stdbool.h>

bool get_window_info(HWND window, char* class, size_t class_size, char* text, size_t text_size, char* path, size_t path_size) {
	if (!GetClassName(window, class, class_size))
		return false;

	if (!GetWindowText(window, text, text_size))
		*text = 0;

	BOOL ret = false;
	{
		DWORD thread_id = GetWindowThreadProcessId(window,NULL); 
		HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id);
		if (!thread) goto end;

		DWORD pid = GetProcessIdOfThread(thread);
		if (!pid) goto end;

		HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (!process) goto end;

		GetProcessImageFileName(process, path, path_size);
		ret = true;
	end:
		if (process) CloseHandle(process);
		if (thread) CloseHandle(thread);
	}
	return ret;
}

