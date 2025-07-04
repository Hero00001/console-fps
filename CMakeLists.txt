cmake_minimum_required(VERSION 3.10)
project(ConsoleFPS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(ConsoleFPS main.cpp)

if(WIN32)
    target_link_libraries(ConsoleFPS PRIVATE kernel32 user32 gdi32)
endif()
