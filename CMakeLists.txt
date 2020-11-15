cmake_minimum_required (VERSION 3.5)
project (Mnome)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
if(NOT MSVC)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Werror -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wall -Wextra -pedantic")
    #set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
endif(NOT MSVC)

set(SOURCE_FILES
    ./src/BeatPlayer.cpp
    ./src/BeatPlayer.hpp
    ./src/Repl.cpp
    ./src/Repl.hpp
    ./src/miniaudio.h
    )

# testable library
add_library(mnomelib STATIC ${SOURCE_FILES})
if ("${CMAKE_HOST_SYSTEM_NAME}" EQUAL "Linux")
    target_link_libraries(mnomelib PRIVATE dl m pthread)
endif ()

add_executable (mnome ./src/main.cpp)
target_link_libraries(mnome mnomelib)


enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(ReplTest ./test/ReplTest.cpp)
target_link_libraries(ReplTest gtest gtest_main mnomelib)
target_include_directories(ReplTest PRIVATE ./src/)
add_test(NAME ReplTest COMMAND ReplTest)
