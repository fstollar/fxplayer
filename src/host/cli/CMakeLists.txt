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

# valgrind/callgrind (3.22) can't decode the AVX-512 that -march=native emits on this CPU and
# aborts with SIGILL. Configure a dedicated dir with -DVALGRIND_BUILD=ON to drop AVX-512 (AVX2
# codegen, which valgrind handles) for profiling instruction counts. OFF for normal builds.
option(VALGRIND_BUILD "Disable AVX-512 so the binary runs under valgrind/callgrind" OFF)
set(ARCH_FLAGS -march=native)
if(VALGRIND_BUILD)
    list(APPEND ARCH_FLAGS -mno-avx512f)
endif()

target_compile_options(fxplayer PRIVATE
    -Wall -Wextra -Wpedantic
    -O3
    ${ARCH_FLAGS}
    -ffast-math
    -funroll-loops
    -flto=auto
    -fwhole-program
    -UNDEBUG   # keep assert() live during experimentation, regardless of build type
)

target_link_options(fxplayer PRIVATE -flto)

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
