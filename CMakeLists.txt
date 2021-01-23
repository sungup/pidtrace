cmake_minimum_required(VERSION 3.17)
project(pidtrace)

set(CMAKE_CXX_STANDARD 14)

set(LINK_LIBRARIES pthread)

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(filesystem HAVE_FILESYSTEM_HEADER)
if (NOT HAVE_FILESYSTEM_HEADER)
  CHECK_INCLUDE_FILE_CXX(experimental/filesystem HAVE_EXP_FILESYSTEM_HEADER)

  if (HAVE_EXP_FILESYSTEM_HEADER)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} stdc++fs)
  else (HAVE_EXP_FILESYSTEM_HEADER)
    message(FATAL_ERROR "pidtrace needs <filesystem> or <experimental/filesystem> header")
  endif (HAVE_EXP_FILESYSTEM_HEADER)
endif (NOT HAVE_FILESYSTEM_HEADER)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(pidtrace src/pidtrace.cpp src/include/process.h src/tracker.cpp src/tracker.h)

target_link_libraries(pidtrace ${LINK_LIBRARIES})
