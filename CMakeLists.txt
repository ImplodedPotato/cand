cmake_minimum_required(VERSION 4.0)
project(cand C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(PLATFORM SDL) # Avoid bug with GLFW startup
add_subdirectory(src/external/raylib)
include_directories(src/external/raygui)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} raylib)
