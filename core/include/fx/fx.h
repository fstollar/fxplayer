/*
 * fx.h - public C API for the F/X module player engine.
 *
 * This API is intentionally minimal at this stage; it grows as the port
 * progresses. The shape is fixed:
 *
 *   - No malloc inside the engine. Callers supply all memory.
 *   - No file I/O. Modules are passed in as a memory buffer.
 *   - No OS headers. C99 fixed-width types only.
 *   - Fully deterministic, sample-accurate timing.
 */
#ifndef FX_FX_H
#define FX_FX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FX_VERSION_MAJOR 0
#define FX_VERSION_MINOR 67
#define FX_VERSION_PATCH 0

typedef enum {
    FX_FORMAT_UNKNOWN = 0,
    FX_FORMAT_MOD,
    FX_FORMAT_S3M,
    FX_FORMAT_669
} fx_format;

typedef enum {
    FX_OUTPUT_S16 = 0,    /* signed 16-bit interleaved */
    FX_OUTPUT_S32         /* signed 32-bit interleaved (raw mix) */
} fx_output_format;

typedef enum {
    FX_OK            =  0,
    FX_ERR_FORMAT    = -1,
    FX_ERR_WORKSPACE = -2,
    FX_ERR_STATE     = -3
} fx_err;

typedef struct {
    uint32_t         sample_rate;
    uint8_t          channels;
    fx_output_format output_format;
    uint8_t          interpolate;  /* 0=nearest, 1=linear */
    uint8_t          soft_clip;    /* 0=hard clip, 1=soft clip */
} fx_config;

/* Opaque player handle. Real type lives in engine/. */
typedef struct fx_player fx_player;

/* Returns "0.67.0" or similar. Always valid, never NULL. */
const char *fx_version_string(void);

/* Sniff a module buffer and report its format. */
fx_format fx_detect_format(const void *data, size_t size);

/*
 * Parse module header; write required workspace bytes to *out_bytes.
 * Call before fx_load to size the workspace buffer.
 * Embedded: call at load time, assert(ws <= MY_STATIC_SIZE).
 */
fx_err fx_workspace_size(const void *data, size_t size, size_t *out_bytes);

/*
 * Load a module from a memory buffer into caller-supplied workspace.
 * workspace is carved internally for pattern+sample data.
 */
fx_err fx_load(const void *data, size_t size,
               void *workspace, size_t ws_size);

/*
 * Render frame_count interleaved frames into out.
 * out must hold frame_count * cfg->channels * sizeof(sample) bytes.
 * Returns frames actually rendered (< frame_count at end of song).
 */
size_t fx_render_frames(void *out, size_t frame_count,
                        const fx_config *cfg);

/* Release engine state. Safe to call even if fx_load failed. */
void fx_close(void);

#ifdef __cplusplus
}
#endif
#endif /* FX_FX_H */
