# fxplayer-cli — command-line player host.

find_package(Threads REQUIRED)

add_executable(fxplayer
    miniaudio.c
    main.cpp
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
