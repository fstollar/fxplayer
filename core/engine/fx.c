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
