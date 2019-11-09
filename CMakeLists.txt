cmake_minimum_required (VERSION 3.5)
project (Mnome)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Werror -Wall -Wextra -pedantic")
set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)

set(SOURCE_FILES
    ./src/BeatPlayer.cpp
    ./src/BeatPlayer.hpp
    ./src/Repl.cpp
    ./src/Repl.hpp
    )

add_library(mnomelib STATIC ${SOURCE_FILES})
target_link_libraries(mnomelib asound pthread)
add_executable (mnome ./src/main.cpp)
target_link_libraries(mnome mnomelib)


enable_testing()
find_package(GTest REQUIRED)
add_executable(ReplTest ./test/ReplTest.cpp)
target_link_libraries(ReplTest GTest::GTest GTest::Main mnomelib)
target_include_directories(ReplTest PRIVATE ./src/)
add_test(NAME ReplTest COMMAND ReplTest)
