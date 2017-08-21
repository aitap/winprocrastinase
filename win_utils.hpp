#define _WIN32_WINNT 0x601
#include <string>
#include <stdexcept>
#include <memory>
#include <windows.h>

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,decltype(&CloseHandle)> u_handle;

std::string get_window_process_path(HWND);
std::string get_window_title(HWND);
void kill_window_process(HWND);
