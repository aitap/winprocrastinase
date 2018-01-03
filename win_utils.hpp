#define _WIN32_WINNT 0x601
#include <string>
#include <stdexcept>
#include <memory>
#include <windows.h>

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,decltype(&CloseHandle)> u_handle;

template<typename T = HANDLE, typename U = decltype(&::CloseHandle), U V = &::CloseHandle>
class hndl {
	T val;
public:
	hndl(T val_) : val(val_) {}
	operator T() { return val; }
	~hndl() { V(val); }
};

using handle = hndl<>;
using hwineventhook = hndl<HWINEVENTHOOK, decltype(&::UnhookWinEvent), &UnhookWinEvent>;
using hkey = hndl<HKEY, decltype(&::RegCloseKey), &RegCloseKey>;

std::string get_window_process_path(HWND);
std::string get_window_title(HWND);
void kill_window_process(HWND);
BY_HANDLE_FILE_INFORMATION get_file_info(const char*);
bool operator==(const BY_HANDLE_FILE_INFORMATION &, const BY_HANDLE_FILE_INFORMATION &);
