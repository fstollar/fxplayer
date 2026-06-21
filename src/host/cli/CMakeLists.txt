# fxplayer-cli — command-line player host.

find_package(Threads REQUIRED)

add_executable(fxplayer
    miniaudio.c
    main.cpp
    tty.cpp
)

target_include_directories(fxplayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(fxplayer PRIVATE
    fx::core
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
