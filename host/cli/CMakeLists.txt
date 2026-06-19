# fxplayer-cli - command-line player host.
#
# Wraps fxcore with a C++ frontend and a cross-platform audio backend
# (miniaudio, planned). For now just prints the engine version.

add_executable(fxplayer
    main.cpp
)

target_link_libraries(fxplayer PRIVATE fx::core)

# TODO: vendor miniaudio under host/cli/third_party/miniaudio/ and add it
# here, then wire up an audio device callback that calls fx_render().
