cmake_minimum_required (VERSION 3.5)
project (Mnome)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic")

set(SOURCE_FILES
   ./src/BeatPlayer.cpp
   ./src/Repl.cpp
	./src/main.cpp
	)

add_executable (mnome ${SOURCE_FILES})
target_link_libraries(mnome asound pthread)
