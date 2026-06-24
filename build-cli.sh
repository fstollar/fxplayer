#!/bin/bash
# build-cli.sh — build the Linux/macOS/Windows CLI host via CMake.
# Run cmake -B build [-DFX_BUILD_TESTS=ON] first to configure.
if [ -x "$(command -v flatpak-spawn)" ]; then
    exec flatpak-spawn --host cmake --build build/ -j$(nproc)
else
    cmake --build build/ -j$(nproc)
fi

