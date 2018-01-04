#define _WIN32_WINNT 0x601
#include <string>
#include <stdexcept>
#include <memory>
#include <windows.h>

template<typename T = HANDLE, typename U = decltype(&::CloseHandle), U V = &::CloseHandle>
class hndl {
	T val;
	hndl& operator= (const hndl&) = delete;
public:
	hndl(T val_) : val(val_) {}
	operator T() { return val; }
	hndl& operator= (T val_) {
		this->~hndl();
		val = val_;
		return *this;
	}
	~hndl() { V(val); }
};

using handle = hndl<>;

std::string get_window_process_path(HWND);
std::string get_window_title(HWND);
void kill_window_process(HWND);

BY_HANDLE_FILE_INFORMATION get_file_info(const char*);
bool operator==(const BY_HANDLE_FILE_INFORMATION &, const BY_HANDLE_FILE_INFORMATION &);

namespace std {
	template<>
	struct hash<BY_HANDLE_FILE_INFORMATION> {
		std::size_t operator()(const BY_HANDLE_FILE_INFORMATION & bhfi) const noexcept {
			// shift then XOR is an acceptable way of combining hashes
			return (
				(
					hash<DWORD>()(bhfi.dwVolumeSerialNumber)
					^ (hash<DWORD>()(bhfi.nFileIndexLow) << 1)
				) >> 1)
			^ (hash<DWORD>()(bhfi.nFileIndexHigh) << 1);
		}
	};
}
