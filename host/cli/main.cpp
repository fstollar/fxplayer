#include <cstdio>

#include "fx/fx.h"

int main(int /*argc*/, char ** /*argv*/)
{
    std::printf("F/X Player %s - cross-platform port (work in progress)\n",
                fx_version_string());
    return 0;
}
