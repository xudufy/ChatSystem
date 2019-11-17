cmake_minimum_required(VERSION 3.0)

project(ChatSystem)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "archive_output" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "runtime_output" FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "library_output" FORCE)

add_compile_options(-std=c++17)

add_definitions(-DSERVER_IP="127.0.0.1" -DSERVER_PORT=61856)

if (NDEBUG) 
    add_compile_options(-O2 -DNDEBUG)
else ()
    add_compile_options(-O0 -g -Wall)
endif()

add_subdirectory(client)
add_subdirectory(server)

add_executable(
    testu

    testu.cpp
    utilities.cpp
)




