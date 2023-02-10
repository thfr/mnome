cmake_minimum_required(VERSION 3.5)
project(Mnome)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 20)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wall -Wextra -pedantic")
else(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
endif(NOT MSVC)

set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)

add_subdirectory(cmake/miniaudio)

set(SOURCE_FILES
    ./src/AudioSignal.cpp
    ./src/AudioSignal.hpp
    ./src/BeatPlayer.cpp
    ./src/BeatPlayer.hpp
    ./src/Mnome.cpp
    ./src/Mnome.hpp
    ./src/Repl.cpp
    ./src/Repl.hpp
)

# testable library
add_library(mnomelib STATIC ${SOURCE_FILES})
target_link_libraries(mnomelib PUBLIC miniaudio_STATIC)

add_executable(mnome ./src/main.cpp)
target_link_libraries(mnome mnomelib)

enable_testing()
FetchContent_Declare(
    googletest
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(ReplTest ./test/ReplTest.cpp)
target_link_libraries(ReplTest gtest gtest_main mnomelib)
target_include_directories(ReplTest PRIVATE ./src/)
add_test(NAME ReplTest COMMAND ReplTest)

add_executable(AudioSignalTest ./test/AudioSignalTest.cpp)
target_link_libraries(AudioSignalTest gtest gtest_main mnomelib)
target_include_directories(AudioSignalTest PRIVATE ./src/)
add_test(NAME AudioSignalTest COMMAND AudioSignalTest)

add_executable(MnomeTest ./test/MnomeTest.cpp)
target_link_libraries(MnomeTest gtest gtest_main mnomelib)
target_include_directories(MnomeTest PRIVATE ./src/)
add_test(NAME MnomeTest COMMAND MnomeTest)
