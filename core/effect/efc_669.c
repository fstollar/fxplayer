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

static void M669_MVibrato(uint32_t ch)
{
    M669_PeriodeAdjust[ch] =
        (int32_t)M669_Vibrato[ch][M669_VibratoPosition[ch]] *
        (int32_t)M669_VibratoDepth[ch] / 64;
    M669_VibratoPosition[ch] =
        (uint16_t)((M669_VibratoPosition[ch] + 1u) & 63u);
}

/* ---- retrig ---- */

static void M669_Retrig(uint32_t ch, int8_t count)
{
    M669_RetrigCount[ch]--;
    if (M669_RetrigCount[ch] == 0) {
        M669_RetrigCount[ch]     = count;
        M669_SamplePosition[ch]  = 0;
        M669_SampleFraction[ch]  = 0;
    }
}

/* ---- global effects (tempo / speed) ---- */

void M669_GlobalEffect(void)
{
    uint32_t ch, effect, info;

    for (ch = 0; ch < M669_CHANNELS; ch++) {
        effect = M669_Effect[ch];
        info   = M669_EffectInfo[ch];

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
    uint32_t ch, effect;

    for (ch = 0; ch < M669_CHANNELS; ch++) {
        effect = M669_Effect[ch];

        switch (effect) {
        case 2u: {  /* glissando — capture target note */
            uint8_t note = M669_RowBuffer[ch*5+0];
            if (note != 0xFFu && note != 0xFEu) {
                M669_PeriodeAdjust[ch] = 0;
                M669_GlissNote[ch]     = note;
                if (note < 64u) {
                    M669_RowBuffer[ch*5+0] = 0xFEu;  /* suppress trigger */
                    M669_GlissFlag[ch]     = 1;
                } else {
                    M669_GlissFlag[ch] = 0;
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
    uint32_t ch, effect, info;

    for (ch = 0; ch < M669_CHANNELS; ch++) {
        effect = M669_Effect[ch];
        info   = M669_EffectInfo[ch];

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* tone slide up — nothing on row tick */ break;
        case 1u:  /* tone slide down — nothing on row tick */ break;

        case 2u:  /* glissando */
            if (info) M669_GlissSpeed[ch] = (uint8_t)info;
            if (M669_GlissFlag[ch] == 1) {
                M669_GlissPeriode[ch] =
                    M669_Periodes[M669_SampleFinetune[ch]][M669_GlissNote[ch]] << 4u;
                M669_GlissFlag[ch] = 2;
            }
            if (M669_GlissPeriode[ch] != M669_Periode[ch]) {
                M669_VibratoPosition[ch] = 0;
                M669_PeriodeAdjust[ch]   = 0;
                M669_GlissFlag[ch]       = 2;
            }
            break;

        case 3u:  /* finetune */
            M669_SampleFinetune[ch] = (uint8_t)info;
            break;

        case 4u:  /* vibrato depth */
            M669_VibratoDepth[ch] = (uint8_t)((info << 4) + 1u);
            break;

        case 6u:  /* balance / panning */
            M669_Panning[ch] = (uint8_t)(info * 28u);
            break;

        case 7u:  /* retrig */
            if (info) M669_RetrigCount[ch] = (int8_t)info;
            break;
        }
    }
}

/* ---- tick effects ---- */

void M669_TickEffect(void)
{
    uint32_t ch, effect, info;

    for (ch = 0; ch < M669_CHANNELS; ch++) {
        effect = M669_Effect[ch];
        info   = M669_EffectInfo[ch];

        /* continued glissando when effect byte is 0xff but flag is set */
        if (effect == 0xFFu && M669_GlissFlag[ch] == 2) effect = 2u;

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* tone slide up */
            M669_Periode[ch] = M669_decPeriode((int32_t)M669_Periode[ch], info * 16u);
            break;

        case 1u:  /* tone slide down */
            M669_Periode[ch] = M669_addPeriode((int32_t)M669_Periode[ch], info * 16u);
            break;

        case 2u:  /* glissando */
            if (M669_GlissPeriode[ch] == M669_Periode[ch] || M669_GlissFlag[ch] != 2) {
                M669_GlissFlag[ch] = 0;
                break;
            }
            if (M669_GlissPeriode[ch] > M669_Periode[ch]) {
                M669_Periode[ch] = M669_addPeriode((int32_t)M669_Periode[ch],
                                                   (uint32_t)M669_GlissSpeed[ch] * 16u);
                if (M669_Periode[ch] > M669_GlissPeriode[ch])
                    M669_Periode[ch] = M669_GlissPeriode[ch];
            } else {
                M669_Periode[ch] = M669_decPeriode((int32_t)M669_Periode[ch],
                                                   (uint32_t)M669_GlissSpeed[ch] * 16u);
                if (M669_Periode[ch] < M669_GlissPeriode[ch])
                    M669_Periode[ch] = M669_GlissPeriode[ch];
            }
            break;

        case 4u:  /* vibrato */
            M669_MVibrato(ch);
            break;

        case 7u:  /* retrig */
            if (info) M669_Retrig(ch, (int8_t)info);
            break;
        }
    }
}

/* ---- init ---- */

void M669_initEffects(void)
{
    uint32_t ch;
    for (ch = 0; ch < M669_CHANNELS; ch++) {
        M669_Vibrato[ch]          = M669_Sinus;
        M669_GlissSpeed[ch]       = 0;
        M669_GlissNote[ch]        = 0;
        M669_GlissPeriode[ch]     = 0;
        M669_GlissFlag[ch]        = 0;
        M669_VibratoDepth[ch]     = 0;
        M669_VibratoPosition[ch]  = 0;
        M669_RetrigCount[ch]      = 0;
    }
}

/* ---- read_row ---- */

void M669_GetNewEffect(void)
{
    uint32_t ch, effect;

    for (ch = 0; ch < M669_CHANNELS; ch++) {
        effect = M669_Effect[ch];

        switch (effect) {
        case 0xFFu:
            M669_PeriodeAdjust[ch] = 0;
            break;
        case 0u:
        case 1u:
            M669_Periode[ch] = (uint32_t)((int32_t)M669_Periode[ch] + M669_PeriodeAdjust[ch]);
            M669_PeriodeAdjust[ch] = 0;
            M669_VibratoPosition[ch] = 0;
            break;
        case 2u:
            M669_VibratoPosition[ch] = 0;
            break;
        }
    }

    M669_GlobalEffect();
    M669_beforeEffect();
}

void M669_read_row(void)
{
    uint32_t ch;
    uint32_t value;

    /* load effects */
    for (ch = 0; ch < M669_CHANNELS; ch++) {
        value = M669_RowBuffer[ch*5+4];
        if (M669_EffectInfo[ch]) M669_LastEffectInfo[ch] = M669_EffectInfo[ch];
        M669_EffectInfo[ch] = (uint8_t)value;

        value = M669_RowBuffer[ch*5+3];
        if (M669_Effect[ch] != 0xFFu) M669_LastEffect[ch] = M669_Effect[ch];
        M669_Effect[ch] = (uint8_t)value;
    }

    M669_GetNewEffect();

    /* note / instrument / volume */
    for (ch = 0; ch < M669_CHANNELS; ch++) {
        value = M669_RowBuffer[ch*5+1];
        if (value <= M669_samples && value != 0xFFu) {
            M669_SampleNr[ch] = (uint8_t)(value + 1u);
            M669_GetNewSample(ch);
        }

        value = M669_RowBuffer[ch*5+0];
        if (value != 0xFFu && value != 0xFEu) {
            M669_Note[ch] = (uint8_t)value;
            M669_GetNewNote(ch);
            M669_VibratoPosition[ch] = 0;
            M669_PeriodeAdjust[ch]   = 0;
        }

        /* volume is set whenever a note event (non-empty row) is present */
        if (M669_RowBuffer[ch*5+0] != 0xFFu) {
            value = M669_RowBuffer[ch*5+2];
            M669_Volume[ch] = (uint8_t)value;
        }
    }

    M669_RowEffect();
}
