cmake_minimum_required(VERSION 3.0)
project(
	WinSelfControl
#	VERSION 0.0.1
	LANGUAGES C
)

if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
	add_compile_options(-std=c99)
endif()

set(ALARM_SOUND ${CMAKE_SOURCE_DIR}/awooga.mp3 CACHE FILEPATH "Path to alarm sound for the control application")
add_custom_command(
	OUTPUT alarm.wav
	MAIN_DEPENDENCY ${ALARM_SOUND}
	COMMAND ffmpeg -i ${ALARM_SOUND} alarm.wav
)
add_custom_target(alarm_wav DEPENDS alarm.wav)

add_library(proc_info proc_info.c)
target_link_libraries(proc_info psapi)

add_executable(create_whitelist list_windows.c)
target_link_libraries(create_whitelist proc_info)

add_executable(control WIN32 controller.c snd.rc)
target_link_libraries(control proc_info winmm)
add_dependencies(control alarm_wav)
