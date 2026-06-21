#ifndef FX_UTIL_CALC_H
#define FX_UTIL_CALC_H

#include <stdint.h>

extern volatile uint32_t g_division;
extern volatile uint32_t g_div_fraction;

uint32_t s3m_calc_speed(uint32_t tempo, uint32_t mix_speed);

void s3m_divide_64bit(uint32_t pos, uint32_t frac,
                      uint32_t div_pos, uint32_t div_frac);

#endif
