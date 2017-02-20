#define _WIN32_WINNT 0x601
#include <windows.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

static BOOL CALLBACK print_window_info(HWND window, LPARAM whitelist) {
	if(!IsWindowVisible(window) || !GetWindowTextLength(window))
		return TRUE;

	char window_class[2048] = {0};
	if (!GetClassName(window, window_class, 2048))
		abort();

	char window_text[2048] = {0};
	if (!GetWindowText(window, window_text, 2048))
		*window_text = 0;

	char module_path[2048] = {0};
	{
		DWORD thread_id = GetWindowThreadProcessId(window,NULL); 
		HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id);
		if (!thread) goto end;

		DWORD pid = GetProcessIdOfThread(thread);
		if (!pid) goto end;

		HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (!process) goto end;

		GetProcessImageFileName(process, module_path, 2048);
	end:
		if (process) CloseHandle(process);
		if (thread) CloseHandle(thread);
	}

	fprintf(
		(FILE*)whitelist,
		"%s\t%s\t%s\n",
		window_class, window_text, module_path
	);

	return TRUE;
}

int main(int argc, char** argv) {
	(void)argc,(void)argv;
	FILE* whitelist = fopen("whitelist.txt", "a+");
	if (!whitelist) abort();

	EnumWindows(print_window_info, (LPARAM)whitelist);

	return 0;
}
