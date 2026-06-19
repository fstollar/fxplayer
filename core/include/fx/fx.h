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

typedef struct {
    uint32_t          sample_rate;     /* e.g. 48000 */
    uint8_t           channels;        /* 1 = mono, 2 = stereo */
    fx_output_format  output_format;
    uint8_t           interpolate;     /* 0 = nearest, 1 = linear */
} fx_config;

/* Opaque player handle. Real type lives in engine/. */
typedef struct fx_player fx_player;

/* Returns "0.67.0" or similar. Always valid, never NULL. */
const char *fx_version_string(void);

/* Sniff a module buffer and report its format. */
fx_format fx_detect_format(const void *data, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* FX_FX_H */
