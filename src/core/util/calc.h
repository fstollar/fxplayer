#ifndef FX_UTIL_CALC_H
#define FX_UTIL_CALC_H

#include <stdint.h>

/* Output registers for s3m_divide_64bit — integer and fractional parts of the result. */
extern volatile uint32_t g_division;
extern volatile uint32_t g_div_fraction;

/* Return the tick length in samples for the given BPM tempo at mix_speed Hz.
   S3M tempo is in BPM; the formula is mix_speed * 2.5 / tempo. */
uint32_t s3m_calc_speed(uint32_t tempo, uint32_t mix_speed);

/*
 * Divide the 64-bit fixed-point value (pos.frac) by (div_pos.div_frac).
 * Result is written to g_division (integer) and g_div_fraction (fraction).
 * Used to compute the per-frame sample position increment from sample rate and pitch.
 */
void s3m_divide_64bit(uint32_t pos, uint32_t frac,
                      uint32_t div_pos, uint32_t div_frac);

#endif
