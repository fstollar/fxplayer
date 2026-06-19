#include "fx/fx.h"

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
    (void)data;
    (void)size;
    return FX_FORMAT_UNKNOWN;
}

fx_err fx_workspace_size(const void *data, size_t size, size_t *out_bytes)
{
    (void)data; (void)size; (void)out_bytes;
    return FX_ERR_STATE;
}

fx_err fx_load(const void *data, size_t size,
               void *workspace, size_t ws_size)
{
    (void)data; (void)size; (void)workspace; (void)ws_size;
    return FX_ERR_STATE;
}

size_t fx_render_frames(void *out, size_t frame_count,
                        const fx_config *cfg)
{
    (void)out; (void)frame_count; (void)cfg;
    return 0;
}

void fx_close(void) {}
