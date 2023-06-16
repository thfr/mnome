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

include(cmake/CPM.cmake)
include(cmake/miniaudio.cmake)

CPMAddPackage("gh:onqtam/doctest@2.4.11")

include_directories(${doctest_SOURCE_DIR}/doctest)

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

add_executable(mnome ./src/main.cpp ${SOURCE_FILES})
target_link_libraries(mnome PUBLIC miniaudio_STATIC doctest)
target_compile_definitions(mnome PUBLIC DOCTEST_CONFIG_DISABLE=1)

enable_testing()
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

add_executable(test-mnome src/doctestmain.cpp ${SOURCE_FILES})
target_link_libraries(test-mnome PUBLIC miniaudio_STATIC doctest)
doctest_discover_tests(test-mnome)
