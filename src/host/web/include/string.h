/*
 * string.h stub for the bare wasm32 build.
 * Implementations live in fxcore_wasm.c.
 */
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
int   memcmp(const void *lhs, const void *rhs, size_t count);

#endif
