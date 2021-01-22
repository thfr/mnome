cmake_minimum_required(VERSION 3.5)
project(Mnome-Miniaudio)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(FetchContent)
FetchContent_Declare(miniaudio # version 0.10.31
                     URL "https://github.com/mackron/miniaudio/archive/53512c39b79ee284058e139604f358fd8a3489cd.zip")
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif(NOT miniaudio_POPULATED)

set(miniaudio_include_dir "${miniaudio_SOURCE_DIR}/extras/miniaudio_split/")
set(miniaudio_sources ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.c
                      ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.h)
add_library(miniaudio_STATIC STATIC ${miniaudio_sources})
target_include_directories(miniaudio_STATIC PUBLIC ${miniaudio_include_dir})
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(miniaudio_STATIC PUBLIC dl m pthread)
endif()
