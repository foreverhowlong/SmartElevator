cmake_minimum_required(VERSION 4.0)
project(SmartElevator)

set(CMAKE_CXX_STANDARD 20)

add_executable(SmartElevator main.cpp
        include/hardware_controller.h)
