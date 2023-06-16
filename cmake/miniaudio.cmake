CPMAddPackage(
    NAME miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio
    # version 0.11.17
    GIT_TAG d76b9a1ac424b5b259c2faeea0dc83d215df522a
    DOWNLOAD_ONLY True
)

set(miniaudio_include_dir "${miniaudio_SOURCE_DIR}/extras/miniaudio_split/")
set(miniaudio_sources ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.c
                      ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.h
)

add_library(miniaudio_STATIC STATIC ${miniaudio_sources})
target_include_directories(miniaudio_STATIC PUBLIC ${miniaudio_include_dir})
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(miniaudio_STATIC PUBLIC dl m pthread)
endif()
