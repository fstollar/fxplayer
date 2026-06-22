#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#include "miniaudio.h"
#include "fx/fx.h"
#include "tty.h"
#include "args.h"

static std::atomic<bool> g_stop{false};

static void on_signal(int) { g_stop = true; tty_raw_disable(); }

struct AudioCtx {
    fx_config            cfg;
    int                  loop;         /* -1=infinite, 0=no loop, N=N repeats */
    uint32_t             prev_loops;   /* fx_song_loops() value last callback */
    std::atomic<bool>    done{false};
    std::atomic<bool>    paused{false};
    std::atomic<int>     vol_cmd{-1};
    std::atomic<uint8_t> cur_vol{64};
    std::atomic<int>     order_cmd{0};
};

static void data_callback(ma_device *dev, void *out, const void * /*in*/, ma_uint32 frame_count)
{
    auto *ctx = static_cast<AudioCtx *>(dev->pUserData);

    int vol = ctx->vol_cmd.exchange(-1, std::memory_order_relaxed);
    if (vol >= 0) {
        fx_set_volume(static_cast<uint8_t>(vol));
        ctx->cur_vol.store(static_cast<uint8_t>(vol), std::memory_order_relaxed);
    }
    int ord = ctx->order_cmd.exchange(0, std::memory_order_relaxed);
    if (ord != 0)
        fx_order_jump(ord);

    if (ctx->paused.load(std::memory_order_relaxed) || ctx->done.load(std::memory_order_relaxed)) {
        std::memset(out, 0, frame_count * ctx->cfg.channels * sizeof(int16_t));
        return;
    }

    size_t rendered = fx_render_frames(out, frame_count, &ctx->cfg);

    /* Zero the tail on a natural end (pattern 255 / order list exhausted). */
    if (rendered < frame_count) {
        size_t tail = (frame_count - rendered) * ctx->cfg.channels * sizeof(int16_t);
        std::memset(static_cast<int16_t *>(out) + rendered * ctx->cfg.channels, 0, tail);
    }

    /*
     * Detect song loops: both Bxx-to-order-0 effects (counter incremented by
     * the effect engine mid-render) and natural ends (pattern 255, also sets
     * rendered < frame_count) bump fx_song_loops(). Check after every render.
     */
    uint32_t cur_loops = fx_song_loops();
    if (cur_loops != ctx->prev_loops) {
        ctx->prev_loops = cur_loops;
        if (ctx->loop == 0) {
            /* No looping: stop on the first loop event. */
            ctx->done = true;
            g_stop    = true;
        } else if (ctx->loop > 0 && (int)cur_loops >= ctx->loop) {
            /* N repeats exhausted. */
            ctx->done = true;
            g_stop    = true;
        } else if (rendered < frame_count) {
            /* Infinite or still has repeats left: un-stick engine after natural end. */
            fx_restart();
        }
        /* Bxx-loop case with repeats remaining: engine already jumped, no action needed. */
    }
}

static uint8_t *read_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) { std::perror(path); return nullptr; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return nullptr; }
    auto *buf = static_cast<uint8_t *>(malloc((size_t)sz));
    if (!buf) { fclose(f); return nullptr; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return nullptr;
    }
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

static const char *format_name(fx_format fmt)
{
    switch (fmt) {
    case FX_FORMAT_S3M: return "S3M";
    case FX_FORMAT_MOD: return "MOD";
    case FX_FORMAT_669: return "669";
    default:            return "???";
    }
}

int main(int argc, char **argv)
{
    FxArgs fxargs;
    if (!fx_parse_args(argc, argv, fxargs))
        return 1;

    size_t   file_size = 0;
    uint8_t *file_buf  = read_file(fxargs.module_path.c_str(), &file_size);
    if (!file_buf) { std::fprintf(stderr, "Cannot read: %s\n", fxargs.module_path.c_str()); return 1; }

    fx_format fmt = fx_detect_format(file_buf, file_size);
    if (fmt == FX_FORMAT_UNKNOWN) {
        std::fprintf(stderr, "Unknown or unsupported module format\n");
        free(file_buf); return 1;
    }

    size_t ws_bytes = 0;
    if (fx_workspace_size(file_buf, file_size, &ws_bytes) != FX_OK) {
        std::fprintf(stderr, "Module header parse failed\n");
        free(file_buf); return 1;
    }

    auto *workspace = static_cast<uint8_t *>(malloc(ws_bytes));
    if (!workspace) {
        std::fprintf(stderr, "Out of memory (workspace %zu bytes)\n", ws_bytes);
        free(file_buf); return 1;
    }

    fx_err err = fx_load(file_buf, file_size, workspace, ws_bytes);
    free(file_buf);
    if (err != FX_OK) {
        std::fprintf(stderr, "fx_load failed: %d\n", (int)err);
        free(workspace); return 1;
    }

    fx_set_volume((uint8_t)fxargs.volume);

    AudioCtx ctx;
    ctx.cfg.sample_rate   = fxargs.sample_rate;
    ctx.cfg.channels      = fxargs.channels;
    ctx.cfg.output_format = FX_OUTPUT_S16;
    ctx.cfg.interpolate   = fxargs.no_interp   ? 0 : 1;
    ctx.cfg.soft_clip     = fxargs.no_softclip ? 0 : 1;
    ctx.loop              = fxargs.loop;
    ctx.prev_loops        = 0;
    ctx.cur_vol.store((uint8_t)fxargs.volume, std::memory_order_relaxed);

    ma_device_config dcfg = ma_device_config_init(ma_device_type_playback);
    dcfg.playback.format   = ma_format_s16;
    dcfg.playback.channels = fxargs.channels;
    dcfg.sampleRate        = fxargs.sample_rate;
    dcfg.dataCallback      = data_callback;
    dcfg.pUserData         = &ctx;

    ma_device device;
    if (ma_device_init(nullptr, &dcfg, &device) != MA_SUCCESS) {
        std::fprintf(stderr, "Failed to open audio device\n");
        fx_close(); free(workspace); return 1;
    }

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    std::printf("F/X Player %s\n", fx_version_string());
    std::printf("Playing: %s  [%s, %u Hz, %s]\n",
        fxargs.module_path.c_str(),
        format_name(fmt),
        fxargs.sample_rate,
        fxargs.channels == 1 ? "mono" : "stereo");
    std::printf("[space] pause  [\xe2\x86\x90/,  \xe2\x86\x92/.] order  [\xe2\x86\x91/+  \xe2\x86\x93/-] volume  [q/Esc] quit\n");

    ma_device_start(&device);
    tty_raw_enable();

    while (!g_stop) {
        switch (tty_read_key()) {
        case FxKey::PAUSE:
            ctx.paused.store(!ctx.paused.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
            break;
        case FxKey::QUIT:
            g_stop = true;
            break;
        case FxKey::ORDER_FWD:
            ctx.order_cmd.store(+1, std::memory_order_relaxed);
            break;
        case FxKey::ORDER_BWD:
            ctx.order_cmd.store(-1, std::memory_order_relaxed);
            break;
        case FxKey::VOL_UP: {
            int v = static_cast<int>(ctx.cur_vol.load(std::memory_order_relaxed)) + 4;
            if (v > 64) v = 64;
            ctx.vol_cmd.store(v, std::memory_order_relaxed);
            break;
        }
        case FxKey::VOL_DOWN: {
            int v = static_cast<int>(ctx.cur_vol.load(std::memory_order_relaxed)) - 4;
            if (v < 0) v = 0;
            ctx.vol_cmd.store(v, std::memory_order_relaxed);
            break;
        }
        default: break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    tty_raw_disable();
    ma_device_uninit(&device);
    fx_close();
    free(workspace);
    std::printf("\n");
    return 0;
}
