cmake_minimum_required(VERSION 3.0)
project(
	Procrastinase
	VERSION 0.0.1
	LANGUAGES CXX
)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	add_compile_options(-std=c++11)
endif()

set(ALARM_SOUND ${CMAKE_SOURCE_DIR}/awooga.mp3 CACHE FILEPATH "Path to alarm sound for the control application")
add_custom_command(
	OUTPUT alarm.wav
	MAIN_DEPENDENCY ${ALARM_SOUND}
	COMMAND ffmpeg -i ${ALARM_SOUND} alarm.wav
)
add_custom_target(alarm_wav DEPENDS alarm.wav)

add_library(win_utils win_utils.cpp)
target_link_libraries(win_utils)

add_executable(control WIN32 controller.cpp snd.rc)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	# we use ordinary int main(int argc, char** argv) for GUI app
	set_target_properties (control PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_link_libraries(control win_utils winmm)
add_dependencies(control alarm_wav)
