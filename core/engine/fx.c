#include <string.h>

#include "fx/fx.h"
#include "format/s3m.h"
#include "mixer/mixer_scalar.h"

#define FX_STR_(x) #x
#define FX_STR(x)  FX_STR_(x)

static const char fx_version_str[] =
    FX_STR(FX_VERSION_MAJOR) "."
    FX_STR(FX_VERSION_MINOR) "."
    FX_STR(FX_VERSION_PATCH);

const char *fx_version_string(void)
{
    return fx_version_str;
}

fx_format fx_detect_format(const void *data, size_t size)
{
    if (size >= 48 &&
        memcmp((const uint8_t *)data + 44, "SCRM", 4) == 0)
        return FX_FORMAT_S3M;
    return FX_FORMAT_UNKNOWN;
}

fx_err fx_workspace_size(const void *data, size_t size, size_t *out_bytes)
{
    size_t ws;
    if (!data || size < 64 || !out_bytes) return FX_ERR_FORMAT;
    ws = s3m_workspace_bytes((const uint8_t *)data, size);
    if (ws == 0) return FX_ERR_FORMAT;
    *out_bytes = ws;
    return FX_OK;
}

fx_err fx_load(const void *data, size_t size,
               void *workspace, size_t ws_size)
{
    int r;
    if (!data || !workspace) return FX_ERR_FORMAT;
    mixer_reset();
    r = s3m_load((const uint8_t *)data, size,
                 (uint8_t *)workspace, ws_size);
    if (r == -2) return FX_ERR_WORKSPACE;
    if (r != 0)  return FX_ERR_FORMAT;
    return FX_OK;
}

size_t fx_render_frames(void *out, size_t frame_count,
                        const fx_config *cfg)
{
    int16_t *pcm = (int16_t *)out;
    size_t   rendered = 0;
    uint32_t chunk;

    if (!out || !cfg || frame_count == 0) return 0;

    /* Apply render config to mixer globals */
    g_MixSpeed         = cfg->sample_rate;
    g_flag_stereo      = (cfg->channels == 2) ? 1u : 0u;
    g_flag_interpolate = cfg->interpolate;
    g_flag_soft_clip   = cfg->soft_clip;

    while (rendered < frame_count) {
        if (s3m_is_done()) break;

        chunk = (uint32_t)(frame_count - rendered);
        if (chunk > FX_INTERNAL_BLOCK) chunk = FX_INTERNAL_BLOCK;

        s3m_render_block(chunk);
        mixer_convert_to_s16(pcm + rendered * cfg->channels,
                             chunk, g_master_vol_table);
        rendered += chunk;
    }
    return rendered;
}

void fx_close(void)
{
    s3m_close();
    mixer_reset();
}
