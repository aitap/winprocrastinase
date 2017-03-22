#define _WIN32_WINNT 0x601
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <windows.h>
#include "win_utils.hpp"

extern "C" BOOL CALLBACK print_window_info(HWND window, LPARAM whitelists) {
	std::ofstream* wl = (std::ofstream*)whitelists;
	if(IsWindowVisible(window) && GetWindowTextLength(window)) {
		wl[0] << get_window_process_path(window) << std::endl;
		wl[1] << get_window_title(window) << std::endl;
	}

	return TRUE;
}

int main() try {
	std::ofstream whitelists[2];
	whitelists[0].open("path_whitelist.txt", std::ios::out|std::ios::app);
	whitelists[1].open("title_whitelist.txt", std::ios::out|std::ios::app);

	if ( !whitelists[0] || !whitelists[1])
		throw std::runtime_error("Error opening whitelists");

	EnumWindows(print_window_info, (LPARAM)&whitelists[0]);
	return 0;
} catch (std::runtime_error & e) {
	std::cout << "Unhandled exception: " << e.what() << std::endl;
	return 1;
}
