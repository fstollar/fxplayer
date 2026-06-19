# fxcore - the engine library.
#
# Pure C99. No malloc, no I/O, no OS headers, no threads.
# The mixer kernel variant is selected at build time via FX_MIXER.

set(FX_MIXER "SCALAR" CACHE STRING
    "Mixer kernel variant: SCALAR | X86_SSE2 | X86_AVX2 | X86_P5_ASM | ARM_DSP")
set_property(CACHE FX_MIXER PROPERTY STRINGS
    SCALAR X86_SSE2 X86_AVX2 X86_P5_ASM ARM_DSP)

set(FXCORE_SOURCES
    engine/fx.c
    # engine/   - channel state, mix dispatch       (TODO)
    # format/   - mod.c s3m.c m669.c wav.c          (TODO)
    # effect/   - efc_mod.c efc_s3m.c efc_669.c     (TODO)
)

if(FX_MIXER STREQUAL "SCALAR")
    list(APPEND FXCORE_SOURCES mixer/mixer_scalar.c)
elseif(FX_MIXER STREQUAL "X86_SSE2")
    list(APPEND FXCORE_SOURCES mixer/mixer_x86_sse2.c)
elseif(FX_MIXER STREQUAL "X86_AVX2")
    list(APPEND FXCORE_SOURCES mixer/mixer_x86_avx2.c)
elseif(FX_MIXER STREQUAL "X86_P5_ASM")
    enable_language(ASM_NASM)
    list(APPEND FXCORE_SOURCES mixer/mixer_x86_p5.asm)
elseif(FX_MIXER STREQUAL "ARM_DSP")
    list(APPEND FXCORE_SOURCES mixer/mixer_armcm_dsp.c)
else()
    message(FATAL_ERROR "Unknown FX_MIXER variant: ${FX_MIXER}")
endif()

add_library(fxcore STATIC ${FXCORE_SOURCES})
add_library(fx::core ALIAS fxcore)

target_include_directories(fxcore
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(fxcore PRIVATE
    FX_MIXER_${FX_MIXER}=1
)

if(MSVC)
    target_compile_options(fxcore PRIVATE /W4)
else()
    target_compile_options(fxcore PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "fxcore: mixer variant = ${FX_MIXER}")
