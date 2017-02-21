#define _WIN32_WINNT 0x601
#include <string>
#include <fstream>
#include <stdexcept>
#include <windows.h>
#include "win_utils.hpp"

extern "C" BOOL CALLBACK print_window_info(HWND window, LPARAM whitelist) {
	if(!IsWindowVisible(window) || !GetWindowTextLength(window))
		return TRUE;

	*((std::ofstream*)whitelist) << get_window_process_path(window) << std::endl;

	return TRUE;
}

int main() {
	std::ofstream whitelist("whitelist.txt", std::ios::out|std::ios::app);
	if (!whitelist) throw std::runtime_error("Error opening whitelist.txt");
	EnumWindows(print_window_info, (LPARAM)&whitelist);
	return 0;
}
