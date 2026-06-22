cmake_minimum_required(VERSION 3.16)

project(fxplayer
    VERSION     0.67.0
    DESCRIPTION "F/X module player - cross-platform port"
    LANGUAGES   C CXX
)

set(CMAKE_C_STANDARD          99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS        OFF)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

include(cmake/get_cpm.cmake)

# valgrind/callgrind (3.22) can't decode the AVX-512 that -march=native emits on this CPU and
# aborts with SIGILL. Configure a dedicated dir with -DVALGRIND_BUILD=ON to drop AVX-512 (AVX2
# codegen, which valgrind handles) for profiling instruction counts. OFF for normal builds.
option(VALGRIND_BUILD "Disable AVX-512 so the binary runs under valgrind/callgrind" OFF)
set(FX_ARCH_FLAGS -march=native)
if(VALGRIND_BUILD)
    list(APPEND FX_ARCH_FLAGS -mno-avx512f)
endif()

option(FX_BUILD_CLI   "Build the C++ CLI host"      ON)
option(FX_BUILD_TESTS "Build unit / regression tests" OFF)

add_subdirectory(src/core)

if(FX_BUILD_CLI)
    add_subdirectory(src/host/cli)
endif()

if(FX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
