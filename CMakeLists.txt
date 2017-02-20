cmake_minimum_required(VERSION 3.0)
project(
	WinSelfControl
#	VERSION 0.0.1
	LANGUAGES C
)

if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
	add_compile_options(-std=c99)
endif()

add_executable(create_whitelist list_windows.c)
target_link_libraries(create_whitelist proc_info)

add_executable(control WIN32 controller.c)
target_link_libraries(control proc_info)
