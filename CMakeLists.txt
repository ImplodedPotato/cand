cmake_minimum_required(VERSION 4.0)
project(cand C)

set(CMAKE_C_STANDARD 11)

set(PLATFORM SDL) # Avoid bug with GLFW startup
add_subdirectory(src/external/raylib)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} raylib)