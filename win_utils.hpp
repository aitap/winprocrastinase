#define _WIN32_WINNT 0x601
#include <string>
#include <stdexcept>
#include <windows.h>
std::string get_window_process_path(HWND); // throws std::runtime_error
std::string get_window_title(HWND); // throws std::runtime_error
bool kill_window_process(HWND);
