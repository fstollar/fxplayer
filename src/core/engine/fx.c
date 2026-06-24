#include <string.h>

#include "fx/fx.h"
#include "format/s3m.h"
#include "format/mod.h"
#include "format/m669.h"
#include "mixer/mixer_scalar.h"

#define FX_STR_(x) #x
#define FX_STR(x)  FX_STR_(x)

static const char fx_version_str[] =
    FX_STR(FX_VERSION_MAJOR) "."
    FX_STR(FX_VERSION_MINOR) "."
    FX_STR(FX_VERSION_PATCH);

static fx_format g_loaded_fmt = FX_FORMAT_UNKNOWN;
static uint8_t   g_ui_volume  = 64;

void fx_init(const fx_config *cfg)
{
    if (!cfg) return;
    g_MixSpeed         = cfg->sample_rate;
    g_flag_stereo      = (cfg->channels == 2) ? 1u : 0u;
    g_flag_interpolate = cfg->interpolate;
    g_flag_soft_clip   = cfg->soft_clip;
}

void fx_set_volume(uint8_t vol)
{
    if (vol > 64) vol = 64;
    g_ui_volume = vol;
    if (g_master_vol_table)
        mixer_calc_master_vol32(g_MasterVolume * vol / 64u, g_master_vol_table);
}

uint8_t fx_get_volume(void)
{
    return g_ui_volume;
}

void fx_order_jump(int delta)
{
    int target;

    switch (g_loaded_fmt) {

    case FX_FORMAT_S3M:
        target = (int)S3M_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)S3M_OrderNum) target = (int)S3M_OrderNum - 1;
        S3M_nextorder = (uint32_t)target;
        S3M_nextrow   = 0;
        S3M_jump      = 1;
        break;

    case FX_FORMAT_MOD:
        target = (int)MOD_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)MOD_OrderNum) target = (int)MOD_OrderNum - 1;
        MOD_nextorder = (uint32_t)target;
        MOD_nextrow   = 0;
        MOD_jump      = 1;
        break;

    case FX_FORMAT_669:
        target = (int)M669_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)M669_OrderNum) target = (int)M669_OrderNum - 1;
        M669_nextorder = (uint32_t)target;
        M669_nextrow   = 0;
        M669_jump      = 1;
        break;

    default:
        break;
    }
}

const char *fx_version_string(void)
{
    return fx_version_str;
}

fx_format fx_detect_format(const void *data, size_t size)
{
    const uint8_t *d = (const uint8_t *)data;

    /* S3M: "SCRM" at offset 44 */
    if (size >= 48u && memcmp(d + 44, "SCRM", 4) == 0)
        return FX_FORMAT_S3M;

    /* 669: magic "if" at offset 0 */
    if (size >= 2u && d[0] == 0x69u && d[1] == 0x66u)
        return FX_FORMAT_669;

    /* MOD: known 4-char ID at offset 1080 (31-sample variants) */
    if (size >= 1084u) {
        static const char *mod_ids[] = {
            "M.K.", "FLT4", "FLT6", "FLT8", "4CHN", "6CHN", "8CHN", "M!K!"
        };
        uint32_t i;
        for (i = 0; i < 8u; i++) {
            if (memcmp(d + 1080, mod_ids[i], 4) == 0)
                return FX_FORMAT_MOD;
        }
    }

    return FX_FORMAT_UNKNOWN;
}

fx_err fx_workspace_size(const void *data, size_t size, size_t *out_bytes)
{
    fx_format fmt;
    size_t ws;

    if (!data || size < 2u || !out_bytes) return FX_ERR_FORMAT;

    fmt = fx_detect_format(data, size);
    switch (fmt) {
    case FX_FORMAT_S3M:
        if (size < 64u) return FX_ERR_FORMAT;
        ws = s3m_workspace_bytes((const uint8_t *)data, size);
        break;
    case FX_FORMAT_MOD:
        ws = mod_workspace_bytes((const uint8_t *)data, size);
        break;
    case FX_FORMAT_669:
        ws = m669_workspace_bytes((const uint8_t *)data, size);
        break;
    default:
        return FX_ERR_FORMAT;
    }

    if (ws == 0) return FX_ERR_FORMAT;
    *out_bytes = ws;
    return FX_OK;
}

fx_err fx_load(const void *data, size_t size,
               void *workspace, size_t ws_size)
{
    fx_format fmt;
    int r;

    if (!data || !workspace) return FX_ERR_FORMAT;

    fmt = fx_detect_format(data, size);
    mixer_reset();
    g_loaded_fmt = FX_FORMAT_UNKNOWN;
    g_ui_volume  = 64;

    switch (fmt) {
    case FX_FORMAT_S3M:
        r = s3m_load((const uint8_t *)data, size,
                     (uint8_t *)workspace, ws_size);
        break;
    case FX_FORMAT_MOD:
        r = mod_load((const uint8_t *)data, size,
                     (uint8_t *)workspace, ws_size);
        break;
    case FX_FORMAT_669:
        r = m669_load((const uint8_t *)data, size,
                      (uint8_t *)workspace, ws_size);
        break;
    default:
        return FX_ERR_FORMAT;
    }

    if (r == -2) return FX_ERR_WORKSPACE;
    if (r != 0)  return FX_ERR_FORMAT;

    g_loaded_fmt = fmt;
    return FX_OK;
}

size_t fx_render_frames(void *out, size_t frame_count,
                        const fx_config *cfg)
{
    int16_t *pcm = (int16_t *)out;
    size_t   rendered = 0;
    uint32_t chunk;

    if (!out || !cfg || frame_count == 0) return 0;

    g_MixSpeed         = cfg->sample_rate;
    g_flag_stereo      = (cfg->channels == 2) ? 1u : 0u;
    g_flag_interpolate = cfg->interpolate;
    g_flag_soft_clip   = cfg->soft_clip;

    while (rendered < frame_count) {
        int done;

        switch (g_loaded_fmt) {
        case FX_FORMAT_S3M:  done = s3m_is_done();  break;
        case FX_FORMAT_MOD:  done = mod_is_done();  break;
        case FX_FORMAT_669:  done = m669_is_done(); break;
        default:             done = 1;              break;
        }
        if (done) break;

        chunk = (uint32_t)(frame_count - rendered);
        if (chunk > FX_INTERNAL_BLOCK) chunk = FX_INTERNAL_BLOCK;

        switch (g_loaded_fmt) {
        case FX_FORMAT_S3M:  s3m_render_block(chunk);  break;
        case FX_FORMAT_MOD:  mod_render_block(chunk);  break;
        case FX_FORMAT_669:  m669_render_block(chunk); break;
        default: break;
        }

        mixer_convert_to_s16(pcm + rendered * cfg->channels,
                             chunk, g_master_vol_table);
        rendered += chunk;
    }
    return rendered;
}

uint32_t fx_song_loops(void)
{
    switch (g_loaded_fmt) {
    case FX_FORMAT_S3M: return s3m_song_loops();
    case FX_FORMAT_MOD: return mod_song_loops();
    case FX_FORMAT_669: return m669_song_loops();
    default:            return 0;
    }
}

const char *fx_song_title(void)
{
    switch (g_loaded_fmt) {
    case FX_FORMAT_S3M: return s3m_song_title();
    case FX_FORMAT_MOD: return mod_song_title();
    case FX_FORMAT_669: return m669_song_title();
    default:            return "";
    }
}

void fx_get_playback_state(fx_playback_state *out)
{
    uint32_t i;
    if (!out) return;
    out->order           = 0;
    out->order_count     = 0;
    out->pattern         = 0;
    out->row             = 0;
    out->row_count       = 64;
    out->channels        = 0;
    out->channels_active = 0;

    switch (g_loaded_fmt) {
    case FX_FORMAT_S3M:
        out->order       = S3M_Order;
        out->order_count = S3M_OrderNum;
        out->pattern     = S3M_Pattern;
        out->row         = S3M_row;
        out->channels    = S3M_channels;
        for (i = 0; i < S3M_channels; i++)
            if (S3M_ChannelActive[i]) out->channels_active++;
        break;
    case FX_FORMAT_MOD:
        out->order       = MOD_Order;
        out->order_count = MOD_OrderNum;
        out->pattern     = MOD_Pattern;
        out->row         = MOD_row;
        out->channels    = MOD_channels;
        for (i = 0; i < MOD_channels; i++)
            if (MOD_ChannelActive[i]) out->channels_active++;
        break;
    case FX_FORMAT_669:
        out->order       = M669_Order;
        out->order_count = M669_OrderNum;
        out->pattern     = M669_Pattern;
        out->row         = M669_row;
        out->channels    = M669_CHANNELS;
        for (i = 0; i < M669_CHANNELS; i++)
            if (M669_ChannelActive[i]) out->channels_active++;
        break;
    default:
        break;
    }
}

void fx_restart(void)
{
    switch (g_loaded_fmt) {
    case FX_FORMAT_S3M: s3m_restart(); break;
    case FX_FORMAT_MOD: mod_restart(); break;
    case FX_FORMAT_669: m669_restart(); break;
    default: break;
    }
}

void fx_close(void)
{
    switch (g_loaded_fmt) {
    case FX_FORMAT_S3M:  s3m_close();  break;
    case FX_FORMAT_MOD:  mod_close();  break;
    case FX_FORMAT_669:  m669_close(); break;
    default: break;
    }
    g_loaded_fmt = FX_FORMAT_UNKNOWN;
    mixer_reset();
}
