cmake_minimum_required(VERSION 3.5)
project(Mnome)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Werror -Wall -Wextra -pedantic")
    set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
endif(NOT MSVC)

include(FetchContent)
FetchContent_Declare(
    miniaudio
    # version 0.10.25
    URL "https://github.com/mackron/miniaudio/raw/140bf99065af08f771f1762604c2c62d60994127/miniaudio.h"
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif(NOT miniaudio_POPULATED)

set(SOURCE_FILES ./src/BeatPlayer.cpp ./src/BeatPlayer.hpp ./src/Repl.cpp ./src/Repl.hpp)

# testable library
add_library(mnomelib STATIC ${SOURCE_FILES})
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(mnomelib PUBLIC dl m pthread)
endif()
target_include_directories(mnomelib SYSTEM PUBLIC ${miniaudio_SOURCE_DIR})

add_executable(mnome ./src/main.cpp)
target_link_libraries(mnome mnomelib)

enable_testing()
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz)
FetchContent_MakeAvailable(googletest)

add_executable(ReplTest ./test/ReplTest.cpp)
target_link_libraries(ReplTest gtest gtest_main mnomelib)
target_include_directories(ReplTest PRIVATE ./src/)
add_test(NAME ReplTest COMMAND ReplTest)
