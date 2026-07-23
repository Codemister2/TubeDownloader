cmake_minimum_required(VERSION 3.22)
project(TurboBallArena VERSION 4.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

add_executable(TurboBallArena src/main.cpp)
target_link_libraries(TurboBallArena PRIVATE raylib)

if(MSVC)
    target_compile_options(TurboBallArena PRIVATE /W4 /permissive- /EHsc)
    target_compile_definitions(TurboBallArena PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(TurboBallArena PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(TurboBallArena PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

install(TARGETS TurboBallArena RUNTIME DESTINATION .)
