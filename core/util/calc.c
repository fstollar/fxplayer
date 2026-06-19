#include "calc.h"

volatile uint32_t g_division     = 0;
volatile uint32_t g_div_fraction = 0;

uint32_t s3m_calc_speed(uint32_t tempo, uint32_t mix_speed)
{
    uint32_t t = ((mix_speed * 10u) << 2) / (tempo * 4u);
    return (t + 2u) >> 2;
}

/*
 * 64/32-bit fixed-point division matching the original Watcom #pragma aux.
 * Replicates: shl ebx,16 / mov bx,cx (build 32-bit divisor)
 *             shld edx,eax,16 / shl eax,16 / mov ax,cx (build 64-bit dividend)
 *             div ebx
 */
void s3m_divide_64bit(uint32_t pos, uint32_t frac,
                      uint32_t div_pos, uint32_t div_frac)
{
    uint32_t divisor  = (div_pos << 16) | (div_frac & 0xFFFFu);
    uint64_t dividend = ((uint64_t)pos << 16) | (frac & 0xFFFFu);
    if (divisor == 0) {
        g_division     = 0xFFFFFFFFu;
        g_div_fraction = 0;
        return;
    }
    g_division     = (uint32_t)(dividend / divisor);
    g_div_fraction = (uint32_t)(dividend % divisor);
}
