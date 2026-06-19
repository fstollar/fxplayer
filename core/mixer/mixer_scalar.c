/*
 * mixer_scalar.c - portable C99 reference mixer.
 *
 * This is the bit-exact ground truth that all other mixer variants
 * (SSE2, AVX2, P5 ASM, Cortex-M DSP) must match sample-for-sample.
 *
 * Implementation pending - placeholder so the library has at least
 * one mixer translation unit when FX_MIXER=SCALAR.
 */

typedef int fx_mixer_scalar_placeholder_;
