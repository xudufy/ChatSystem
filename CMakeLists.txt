cmake_minimum_required(VERSION 3.0)

project(ChatSystem C CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "archive_output" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "runtime_output" FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE STRING "library_output" FORCE)

find_package(Threads REQUIRED)

add_compile_options(-pthread -std=c++11)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DSERVER_IP="127.0.0.1" -DSERVER_PORT=65502)

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




