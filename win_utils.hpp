#include <string>
#include <stdexcept>
std::string get_window_process_path(HWND); // throws std::runtime_error
std::string get_window_title(HWND); // throws std::runtime_error
bool kill_window_process(HWND);
