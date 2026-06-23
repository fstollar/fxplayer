#include <stdint.h>
#include <stddef.h>
#include "format/m669.h"
#include "effect/efc_669.h"
#include "mixer/mixer_scalar.h"
#include "util/calc.h"

/* ---- sinus table (shared with MOD) ---- */

static const int16_t M669_Sinus[64] = {
       0,  100,  200,  297,  392,  483,  569,  650,
     724,  792,  851,  903,  946,  980, 1004, 1019,
    1024, 1019, 1004,  980,  946,  903,  851,  792,
     724,  650,  569,  483,  392,  297,  200,  100,
       0, -100, -200, -297, -392, -483, -569, -650,
    -724, -792, -851, -903, -946, -980,-1004,-1019,
   -1024,-1019,-1004, -980, -946, -903, -851, -792,
    -724, -650, -569, -483, -392, -297, -200, -100
};

/* ---- per-channel effect state ---- */

static const int16_t *M669_Vibrato[M669_CHANNELS];

/* Glissando */
static uint8_t  M669_GlissSpeed[M669_CHANNELS];
static uint8_t  M669_GlissNote[M669_CHANNELS];
       uint32_t M669_GlissPeriode[M669_CHANNELS];
       uint8_t  M669_GlissFlag[M669_CHANNELS];

/* Vibrato */
static uint8_t  M669_VibratoDepth[M669_CHANNELS];
       uint16_t M669_VibratoPosition[M669_CHANNELS];

/* Retrig */
static int8_t   M669_RetrigCount[M669_CHANNELS];

/* ---- period helpers ---- */

static uint32_t M669_addPeriode(int32_t p, uint32_t value)
{
    p += (int32_t)value;
    if (p < 10) p = 10;
    return (uint32_t)p;
}

static uint32_t M669_decPeriode(int32_t p, uint32_t value)
{
    p -= (int32_t)value;
    if (p < 10) p = 10;
    return (uint32_t)p;
}

/* ---- vibrato ---- */

static void M669_MVibrato(uint32_t channel_index)
{
    M669_PeriodeAdjust[channel_index] =
        (int32_t)M669_Vibrato[channel_index][M669_VibratoPosition[channel_index]] *
        (int32_t)M669_VibratoDepth[channel_index] / 64;
    M669_VibratoPosition[channel_index] =
        (uint16_t)((M669_VibratoPosition[channel_index] + 1u) & 63u);
}

/* ---- retrig ---- */

static void M669_Retrig(uint32_t channel_index, int8_t count)
{
    M669_RetrigCount[channel_index]--;
    if (M669_RetrigCount[channel_index] == 0) {
        M669_RetrigCount[channel_index]     = count;
        M669_SamplePosition[channel_index]  = 0;
        M669_SampleFraction[channel_index]  = 0;
    }
}

/* ---- global effects (tempo / speed) ---- */

void M669_GlobalEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        effect = M669_Effect[channel_index];
        info   = M669_EffectInfo[channel_index];

        switch (effect) {
        case 0xFFu: break;
        case 5u:  /* set speed */
            if (info) M669_speed = (uint8_t)info;
            break;
        }
    }
}

/* ---- before-effect (glissando preparation) ---- */

void M669_beforeEffect(void)
{
    uint32_t channel_index, effect;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        effect = M669_Effect[channel_index];

        switch (effect) {
        case 2u: {  /* glissando — capture target note */
            uint8_t note = M669_RowBuffer[channel_index*5+0];
            if (note != 0xFFu && note != 0xFEu) {
                M669_PeriodeAdjust[channel_index] = 0;
                M669_GlissNote[channel_index]     = note;
                if (note < 64u) {
                    M669_RowBuffer[channel_index*5+0] = 0xFEu;  /* suppress trigger */
                    M669_GlissFlag[channel_index]     = 1;
                } else {
                    M669_GlissFlag[channel_index] = 0;
                }
            }
            break;
        }
        }
    }
}

/* ---- row effects ---- */

void M669_RowEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        effect = M669_Effect[channel_index];
        info   = M669_EffectInfo[channel_index];

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* tone slide up — nothing on row tick */ break;
        case 1u:  /* tone slide down — nothing on row tick */ break;

        case 2u:  /* glissando */
            if (info) M669_GlissSpeed[channel_index] = (uint8_t)info;
            if (M669_GlissFlag[channel_index] == 1) {
                M669_GlissPeriode[channel_index] =
                    M669_Periodes[M669_SampleFinetune[channel_index]][M669_GlissNote[channel_index]] << 4u;
                M669_GlissFlag[channel_index] = 2;
            }
            if (M669_GlissPeriode[channel_index] != M669_Periode[channel_index]) {
                M669_VibratoPosition[channel_index] = 0;
                M669_PeriodeAdjust[channel_index]   = 0;
                M669_GlissFlag[channel_index]       = 2;
            }
            break;

        case 3u:  /* finetune */
            M669_SampleFinetune[channel_index] = (uint8_t)info;
            break;

        case 4u:  /* vibrato depth */
            M669_VibratoDepth[channel_index] = (uint8_t)((info << 4) + 1u);
            break;

        case 6u:  /* balance / panning */
            M669_Panning[channel_index] = (uint8_t)(info * 28u);
            break;

        case 7u:  /* retrig */
            if (info) M669_RetrigCount[channel_index] = (int8_t)info;
            break;
        }
    }
}

/* ---- tick effects ---- */

void M669_TickEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        effect = M669_Effect[channel_index];
        info   = M669_EffectInfo[channel_index];

        /* continued glissando when effect byte is 0xff but flag is set */
        if (effect == 0xFFu && M669_GlissFlag[channel_index] == 2) effect = 2u;

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* tone slide up */
            M669_Periode[channel_index] = M669_decPeriode((int32_t)M669_Periode[channel_index], info * 16u);
            break;

        case 1u:  /* tone slide down */
            M669_Periode[channel_index] = M669_addPeriode((int32_t)M669_Periode[channel_index], info * 16u);
            break;

        case 2u:  /* glissando */
            if (M669_GlissPeriode[channel_index] == M669_Periode[channel_index] || M669_GlissFlag[channel_index] != 2) {
                M669_GlissFlag[channel_index] = 0;
                break;
            }
            if (M669_GlissPeriode[channel_index] > M669_Periode[channel_index]) {
                M669_Periode[channel_index] = M669_addPeriode((int32_t)M669_Periode[channel_index],
                                                   (uint32_t)M669_GlissSpeed[channel_index] * 16u);
                if (M669_Periode[channel_index] > M669_GlissPeriode[channel_index])
                    M669_Periode[channel_index] = M669_GlissPeriode[channel_index];
            } else {
                M669_Periode[channel_index] = M669_decPeriode((int32_t)M669_Periode[channel_index],
                                                   (uint32_t)M669_GlissSpeed[channel_index] * 16u);
                if (M669_Periode[channel_index] < M669_GlissPeriode[channel_index])
                    M669_Periode[channel_index] = M669_GlissPeriode[channel_index];
            }
            break;

        case 4u:  /* vibrato */
            M669_MVibrato(channel_index);
            break;

        case 7u:  /* retrig */
            if (info) M669_Retrig(channel_index, (int8_t)info);
            break;
        }
    }
}

/* ---- init ---- */

void M669_initEffects(void)
{
    uint32_t channel_index;
    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        M669_Vibrato[channel_index]          = M669_Sinus;
        M669_GlissSpeed[channel_index]       = 0;
        M669_GlissNote[channel_index]        = 0;
        M669_GlissPeriode[channel_index]     = 0;
        M669_GlissFlag[channel_index]        = 0;
        M669_VibratoDepth[channel_index]     = 0;
        M669_VibratoPosition[channel_index]  = 0;
        M669_RetrigCount[channel_index]      = 0;
    }
}

/* ---- read_row ---- */

void M669_GetNewEffect(void)
{
    uint32_t channel_index, effect;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        effect = M669_Effect[channel_index];

        switch (effect) {
        case 0xFFu:
            M669_PeriodeAdjust[channel_index] = 0;
            break;
        case 0u:
        case 1u:
            M669_Periode[channel_index] = (uint32_t)((int32_t)M669_Periode[channel_index] + M669_PeriodeAdjust[channel_index]);
            M669_PeriodeAdjust[channel_index] = 0;
            M669_VibratoPosition[channel_index] = 0;
            break;
        case 2u:
            M669_VibratoPosition[channel_index] = 0;
            break;
        }
    }

    M669_GlobalEffect();
    M669_beforeEffect();
}

void M669_read_row(void)
{
    uint32_t channel_index;
    uint32_t value;

    /* load effects */
    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        value = M669_RowBuffer[channel_index*5+4];
        if (M669_EffectInfo[channel_index]) M669_LastEffectInfo[channel_index] = M669_EffectInfo[channel_index];
        M669_EffectInfo[channel_index] = (uint8_t)value;

        value = M669_RowBuffer[channel_index*5+3];
        if (M669_Effect[channel_index] != 0xFFu) M669_LastEffect[channel_index] = M669_Effect[channel_index];
        M669_Effect[channel_index] = (uint8_t)value;
    }

    M669_GetNewEffect();

    /* note / instrument / volume */
    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        value = M669_RowBuffer[channel_index*5+1];
        if (value <= M669_samples && value != 0xFFu) {
            M669_SampleNr[channel_index] = (uint8_t)(value + 1u);
            M669_GetNewSample(channel_index);
        }

        value = M669_RowBuffer[channel_index*5+0];
        if (value != 0xFFu && value != 0xFEu) {
            M669_Note[channel_index] = (uint8_t)value;
            M669_GetNewNote(channel_index);
            M669_VibratoPosition[channel_index] = 0;
            M669_PeriodeAdjust[channel_index]   = 0;
        }

        /* volume is set whenever a note event (non-empty row) is present */
        if (M669_RowBuffer[channel_index*5+0] != 0xFFu) {
            value = M669_RowBuffer[channel_index*5+2];
            M669_Volume[channel_index] = (uint8_t)value;
        }
    }

    M669_RowEffect();
}
