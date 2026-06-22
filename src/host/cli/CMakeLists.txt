# fxplayer-cli — command-line player host.

CPMAddPackage(
    NAME miniaudio
    GITHUB_REPOSITORY mackron/miniaudio
    GIT_TAG 0.11.21
    DOWNLOAD_ONLY YES
)
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})

CPMAddPackage("gh:jarro2783/cxxopts@3.2.0")

find_package(Threads REQUIRED)

add_executable(fxplayer
    miniaudio.c
    main.cpp
    tty.cpp
)

target_compile_options(fxplayer PRIVATE
    -Wall -Wextra -Wpedantic
    -O3
    ${FX_ARCH_FLAGS}
    -funroll-loops
    -flto=auto
    -UNDEBUG   # keep assert() live during experimentation, regardless of build type
)

target_link_options(fxplayer PRIVATE -flto=auto)

target_include_directories(fxplayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fxplayer PRIVATE
    fx::core
    miniaudio
    cxxopts::cxxopts
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m
)

# Copy the binary to release/ at the repo root after every successful build.
add_custom_command(TARGET fxplayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/release
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:fxplayer> ${CMAKE_SOURCE_DIR}/release/fxplayer
    COMMENT "Copied fxplayer -> release/"
)
