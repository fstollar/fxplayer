/*
 * fxcore_wasm.c — Wasm shim for the F/X engine.
 *
 * Provides static memory arenas, libc polyfills (memset/memcpy/memcmp),
 * and thin wrapper functions the AudioWorklet JS host calls directly.
 * Compiled with -nostdlib -fno-builtin; no libc is linked.
 */
#include <stddef.h>
#include <stdint.h>
#include "fx/fx.h"

/* ── libc polyfills ─────────────────────────────────────────────────── */

void *memset(void *dest, int value, size_t count)
{
    uint8_t *ptr = (uint8_t *)dest;
    for (size_t index = 0; index < count; index++)
        ptr[index] = (uint8_t)value;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    uint8_t       *dst_ptr = (uint8_t *)dest;
    const uint8_t *src_ptr = (const uint8_t *)src;
    for (size_t index = 0; index < count; index++)
        dst_ptr[index] = src_ptr[index];
    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const uint8_t *left_ptr  = (const uint8_t *)lhs;
    const uint8_t *right_ptr = (const uint8_t *)rhs;
    for (size_t index = 0; index < count; index++)
    {
        if (left_ptr[index] != right_ptr[index])
            return (int)left_ptr[index] - (int)right_ptr[index];
    }
    return 0;
}

/* ── static arenas ──────────────────────────────────────────────────── */

/*
 * All three regions are zero-initialized (.bss) — they don't bloat the
 * .wasm binary; the Wasm runtime allocates them in linear memory at startup.
 */
#define MODULE_BUF_SIZE  (4 * 1024 * 1024)   /* 4 MB: raw module bytes    */
#define WORKSPACE_SIZE   (4 * 1024 * 1024)   /* 4 MB: engine workspace    */
#define RENDER_FRAMES    2048                 /* max frames per render call */

static uint8_t g_module_buf[MODULE_BUF_SIZE];
static uint8_t g_workspace[WORKSPACE_SIZE];
static int16_t g_render_buf[RENDER_FRAMES * 2];   /* stereo interleaved */

static fx_config g_config = {
    .sample_rate   = 44100,
    .channels      = 2,
    .output_format = FX_OUTPUT_S16,
    .interpolate   = 1,
    .soft_clip     = 1,
};

static fx_playback_state g_state;

/* ── module input ───────────────────────────────────────────────────── */

uint8_t  *wasm_module_buf(void)      { return g_module_buf; }
uint32_t  wasm_module_buf_size(void) { return MODULE_BUF_SIZE; }

/*
 * Call after writing module bytes into wasm_module_buf().
 * Returns fx_err (0 = FX_OK).
 */
int32_t wasm_load(uint32_t data_size)
{
    size_t ws_bytes;
    fx_err err = fx_workspace_size(g_module_buf, data_size, &ws_bytes);
    if (err != FX_OK)
        return (int32_t)err;
    if (ws_bytes > WORKSPACE_SIZE)
        return (int32_t)FX_ERR_WORKSPACE;
    /* Apply audio config so format loaders see the correct mix rate. */
    fx_init(&g_config);
    return (int32_t)fx_load(g_module_buf, data_size, g_workspace, ws_bytes);
}

/* ── audio config ───────────────────────────────────────────────────── */

/*
 * Call once after init, before the first wasm_render().
 * sample_rate comes from AudioContext.sampleRate (44100 or 48000).
 */
void wasm_set_config(uint32_t sample_rate, uint32_t interpolate, uint32_t soft_clip)
{
    g_config.sample_rate = sample_rate;
    g_config.interpolate = (uint8_t)interpolate;
    g_config.soft_clip   = (uint8_t)soft_clip;
}

/* ── render ─────────────────────────────────────────────────────────── */

int16_t  *wasm_render_buf(void)        { return g_render_buf; }
uint32_t  wasm_render_buf_frames(void) { return RENDER_FRAMES; }

/*
 * Render up to frame_count stereo frames into g_render_buf.
 * Returns frames actually written; < frame_count means song ended.
 */
uint32_t wasm_render(uint32_t frame_count)
{
    if (frame_count > RENDER_FRAMES)
        frame_count = RENDER_FRAMES;
    return (uint32_t)fx_render_frames(g_render_buf, frame_count, &g_config);
}

/* ── playback state ─────────────────────────────────────────────────── */

/*
 * Populate g_state from the engine, then read it via wasm_state_ptr().
 * JS reads the 7 uint32_t fields as a Uint32Array at the returned offset.
 *
 * Field order matches fx_playback_state:
 *   [0]=order  [1]=order_count  [2]=pattern  [3]=row
 *   [4]=row_count  [5]=channels  [6]=channels_active
 */
void               wasm_get_state(void)  { fx_get_playback_state(&g_state); }
fx_playback_state *wasm_state_ptr(void)  { return &g_state; }
