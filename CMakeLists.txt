cmake_minimum_required(VERSION 4.0)
project(ACE_Disappear C)

set(CMAKE_C_STANDARD 11)

# GUI 程序，使用 UNICODE 入口
add_executable(ACE_Disappear WIN32 main.c admin.manifest)
target_link_libraries(ACE_Disappear shlwapi shell32)

# 仅此一句，MinGW 会自动选 wWinMainCRTStartup
set_target_properties(ACE_Disappear PROPERTIES
        LINK_FLAGS "-municode")