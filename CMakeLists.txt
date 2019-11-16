cmake_minimum_required(VERSION 3.0)

project(ChatSystem)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "archive_output" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "runtime_output" FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "library_output" FORCE)

add_compile_options(-std=c++17)

if (NDEBUG) 
    add_compile_options(-O2 -DNDEBUG)
else ()
    add_compile_options(-O0 -g -Wall)
endif()

add_subdirectory(client)
add_subdirectory(server)





