cmake_minimum_required(VERSION 3.17)
project(pidtrace)

FIND_PACKAGE( Boost COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(pidtrace src/pidtrace.cpp src/include/process.h src/tracker.cpp src/tracker.h)

target_link_libraries(pidtrace pthread ${Boost_LIBRARIES})