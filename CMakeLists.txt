cmake_minimum_required(VERSION 3.21)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS OR GEODE_TARGET_PLATFORM STREQUAL "iOS")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(AutoDeco VERSION 1.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Run 'geode sdk install' and restart Terminal.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
