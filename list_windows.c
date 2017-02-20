#define _WIN32_WINNT 0x601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "proc_info.h"

static BOOL CALLBACK print_window_info(HWND window, LPARAM whitelist) {
	if(!IsWindowVisible(window) || !GetWindowTextLength(window))
		return TRUE;

	char window_class[2048] = {0}, window_text[2048] = {0}, module_path[2048] = {0};
	if (!get_window_info(window, window_class, 2048, window_text, 2048, module_path, 2048))
		abort();

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
