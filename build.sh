#!/bin/bash
if [ -x "$(command -v flatpak-spawn)" ]; then
    exec flatpak-spawn --host cmake --build build/ -j$(nproc)
else
    cmake --build build/ -j$(nproc)
fi

