#include "args.h"
#include <cstdio>
#include <cxxopts.hpp>
#include "fx/fx.h"

bool fx_parse_args(int argc, char **argv, FxArgs &out)
{
    cxxopts::Options opts("fxplayer", "F/X Player " + std::string(fx_version_string()));
    opts.add_options()
        ("r,rate",           "Sample rate in Hz",                              cxxopts::value<uint32_t>()->default_value("48000"))
        ("c,channels",       "Output channels: mono or stereo",                cxxopts::value<std::string>()->default_value("stereo"))
        ("l,loop",           "Loop: -1=infinite, 0=no loop, N=N repeats",     cxxopts::value<int>()->default_value("-1"))
        ("no-interpolation", "Disable linear interpolation (nearest-neighbor)")
        ("no-softclip",      "Disable soft clipping (hard clip instead)")
        ("v,volume",         "Master volume 1-64",                             cxxopts::value<int>()->default_value("64"))
        ("h,help",           "Show this help")
        ("module",           "Module file (.s3m / .mod / .669)",               cxxopts::value<std::string>());
    opts.parse_positional({"module"});
    opts.positional_help("<module>");

    cxxopts::ParseResult args;
    try {
        args = opts.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception &e) {
        std::fprintf(stderr, "Error: %s\n\n%s\n", e.what(), opts.help().c_str());
        return false;
    }

    if (args.count("help")) {
        std::printf("%s\n", opts.help().c_str());
        std::exit(0);
    }
    if (!args.count("module")) {
        std::printf("%s\n", opts.help().c_str());
        return false;
    }

    uint32_t    sample_rate = args["rate"].as<uint32_t>();
    std::string chan_str    = args["channels"].as<std::string>();
    int         loop        = args["loop"].as<int>();
    int         volume      = args["volume"].as<int>();

    if (sample_rate < 8000 || sample_rate > 192000) {
        std::fprintf(stderr, "Error: sample rate must be 8000-192000 Hz\n");
        return false;
    }
    if (chan_str != "mono" && chan_str != "stereo") {
        std::fprintf(stderr, "Error: --channels must be 'mono' or 'stereo'\n");
        return false;
    }
    if (volume < 1 || volume > 64) {
        std::fprintf(stderr, "Error: volume must be 1-64\n");
        return false;
    }

    out.module_path = args["module"].as<std::string>();
    out.sample_rate = sample_rate;
    out.channels    = (chan_str == "mono") ? 1 : 2;
    out.loop        = loop;
    out.volume      = volume;
    out.no_interp   = args.count("no-interpolation") > 0;
    out.no_softclip = args.count("no-softclip") > 0;
    return true;
}
