cmake_minimum_required(VERSION 3.5)
project(Mnome-Miniaudio)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(FetchContent)
# https://github.com/mackron/miniaudio/
FetchContent_Declare(
    miniaudio # version 0.11.11
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL "https://github.com/mackron/miniaudio/archive/a0dc1037f99a643ff5fad7272cd3d6461f2d63fa.zip"
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif(NOT miniaudio_POPULATED)

set(miniaudio_include_dir "${miniaudio_SOURCE_DIR}/extras/miniaudio_split/")
set(miniaudio_sources ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.c
                      ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.h
)
add_library(miniaudio_STATIC STATIC ${miniaudio_sources})
target_include_directories(miniaudio_STATIC PUBLIC ${miniaudio_include_dir})
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(miniaudio_STATIC PUBLIC dl m pthread)
endif()
