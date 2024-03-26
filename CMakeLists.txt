# CMakeList.txt : CMake project for NekoNet, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.13)

include(toolchain.cmake)
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/../pico-sdk)
set(PICO_BOARD "pico_w")
set(PICO_PLATFORM "rp2040")
set(PICO_COMPILER "pico_arm_gcc")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(pico_sdk_import.cmake)

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(NekoNet C CXX ASM)

pico_sdk_init()

add_subdirectory(inc)
add_subdirectory(src)
