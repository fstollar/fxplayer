#pragma once
#include <cstdint>
#include <string>

struct FxArgs {
    std::string module_path;
    uint32_t    sample_rate;
    uint8_t     channels;    /* 1=mono, 2=stereo */
    int         loop;        /* -1=infinite, 0=no loop, N=N repeats */
    int         volume;      /* 1-64 */
    bool        no_interp;
    bool        no_softclip;
};

/* Parses argv into out. Prints usage/error and returns false on failure or --help. */
bool fx_parse_args(int argc, char **argv, FxArgs &out);
