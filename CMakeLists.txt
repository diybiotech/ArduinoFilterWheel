cmake_minimum_required(VERSION 3.6)
project(ArduinoFilterWheel)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    ArduinoHub.cpp
    ArduinoHub.h
    FilterWheel.cpp
    FilterWheel.h
    license.txt
    Makefile.am
    ArduinoFilterWheel.vcxproj
    ArduinoFilterWheel.vcxproj.filters)

add_executable(ArduinoFilterWheel ${SOURCE_FILES})