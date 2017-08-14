#define _WIN32_WINNT 0x601
#include <string>
#include <stdexcept>
#include <windows.h>
std::string get_window_process_path(HWND);
std::string get_window_title(HWND);
void kill_window_process(HWND);
