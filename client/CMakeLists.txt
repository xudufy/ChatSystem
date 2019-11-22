find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

# <XPREFIX>_LIBRARIES      ... only the libraries (w/o the '-l')
# <XPREFIX>_LIBRARY_DIRS   ... the paths of the libraries (w/o the '-L')
# <XPREFIX>_LDFLAGS        ... all required linker flags
# <XPREFIX>_LDFLAGS_OTHER  ... all other linker flags
# <XPREFIX>_INCLUDE_DIRS   ... the '-I' preprocessor flags (w/o the '-I')
# <XPREFIX>_CFLAGS         ... all required cflags
# <XPREFIX>_CFLAGS_OTHER   ... the other compiler flags

add_compile_options(${GTK_CFLAGS})
link_directories(${GTK_LIBRARY_DIRS})
link_libraries(${GTK_LIBRARIES})

add_library(
    clientNet
    
    OBJECT
    
    clientNet.cpp
    ../utilities.cpp
)

add_executable(
    chatClient
    
    clientGui.cpp
    $<TARGET_OBJECTS:clientNet>
)



add_executable(
    testClientNet

    test.cpp
    $<TARGET_OBJECTS:clientNet>
)

add_custom_target(clientuixml ALL COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/client.glade ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DEPENDS client.glade)
