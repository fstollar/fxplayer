#include <stdint.h>
#include <stddef.h>
#include "format/mod.h"
#include "effect/efc_mod.h"
#include "mixer/mixer_scalar.h"
#include "util/calc.h"

/* ---- waveform tables ---- */

static const int16_t MOD_Sinus[64] = {
       0,  100,  200,  297,  392,  483,  569,  650,
     724,  792,  851,  903,  946,  980, 1004, 1019,
    1024, 1019, 1004,  980,  946,  903,  851,  792,
     724,  650,  569,  483,  392,  297,  200,  100,
       0, -100, -200, -297, -392, -483, -569, -650,
    -724, -792, -851, -903, -946, -980,-1004,-1019,
   -1024,-1019,-1004, -980, -946, -903, -851, -792,
    -724, -650, -569, -483, -392, -297, -200, -100
};

static const int16_t MOD_Ramp[64] = {
   -1024, -992, -960, -928, -896, -864, -832, -800,
    -768, -736, -704, -672, -640, -608, -576, -544,
    -512, -480, -448, -416, -384, -352, -320, -288,
    -256, -224, -192, -160, -128,  -96,  -64,  -32,
       0,   32,   64,   96,  128,  160,  192,  224,
     256,  288,  320,  352,  384,  416,  448,  480,
     512,  544,  576,  608,  640,  672,  704,  736,
     768,  800,  832,  864,  896,  928,  960,  992
};

static const int16_t MOD_Square[64] = {
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int16_t MOD_Random[64] = {
     235,  761, -875,  565,  677,  789, -557, -349,
     905,  378, -378,  480,  609, -426, -140, -801,
     730,  -48,  807, -134,   60,  932,  417, -315,
     512,  869, -184, 1006,-1024,  551, -311,  110,
       8, -415,  449,   62,  637, -340,  -39, -291,
     -27, -963,   35,  -33,  447, -740,  907,  -50,
    -149,  350,   62,  562, -430,  606,  410,  484,
    -252,  405,  -64, -314, -272, -578, -972,  101
};

/* ---- per-channel effect state ---- */

static const int16_t *MOD_Vibrato[MOD_MAXCHANNELS];
static const int16_t *MOD_Tremolo[MOD_MAXCHANNELS];

/* Glissando */
static uint8_t  MOD_GlissSpeed[MOD_MAXCHANNELS];
static uint8_t  MOD_GlissNote[MOD_MAXCHANNELS];
       uint32_t MOD_GlissPeriod[MOD_MAXCHANNELS];
static uint8_t  MOD_GlissFlag[MOD_MAXCHANNELS];

/* Vibrato */
static uint8_t  MOD_VibratoSpeed[MOD_MAXCHANNELS];
static uint8_t  MOD_VibratoDepth[MOD_MAXCHANNELS];
       uint16_t MOD_VibratoPosition[MOD_MAXCHANNELS];

/* Tremolo */
       uint16_t MOD_TremoloVolume[MOD_MAXCHANNELS];

/* Retrig */
static int8_t   MOD_RetrigCount[MOD_MAXCHANNELS];

/* Note-delay */
static uint8_t  MOD_NoteDelayFlag[MOD_MAXCHANNELS];
static uint8_t  MOD_NoteDelayNote[MOD_MAXCHANNELS];
static uint8_t  MOD_NoteDelayInst[MOD_MAXCHANNELS];

/* Pattern loop */
static int16_t  MOD_PatLoopCount  = 0;
static uint32_t MOD_PatLoopRow    = 0;

/* ---- volume helpers ---- */

static uint16_t MOD_addVolume(int16_t vol, int16_t value)
{
    vol += value;
    if (vol > 64) vol = 64;
    if (vol < 0)  vol = 0;
    return (uint16_t)vol;
}

static uint16_t MOD_decVolume(int16_t vol, int16_t value)
{
    vol -= value;
    if (vol < 0)  vol = 0;
    if (vol > 64) vol = 64;
    return (uint16_t)vol;
}

/* ---- period slide helpers ---- */

static uint32_t MOD_addPeriod(int32_t p, uint32_t value)
{
    p += (int32_t)value;
    if (MOD_flag_amigalimits) {
        if (p < 113) p = 113;
        if (p > 856) p = 856;
    } else {
        if (p < 10) p = 10;
    }
    return (uint32_t)p;
}

static uint32_t MOD_decPeriod(int32_t p, uint32_t value)
{
    p -= (int32_t)value;
    if (MOD_flag_amigalimits) {
        if (p < 113) p = 113;
        if (p > 856) p = 856;
    } else {
        if (p < 10) p = 10;
    }
    return (uint32_t)p;
}

/* ---- vibrato / tremolo ---- */

static void MOD_MVibrato(uint32_t channel_index)
{
    if (MOD_VibratoSpeed[channel_index] == 0) return;
    MOD_PeriodAdjust[channel_index] =
        (int32_t)MOD_Vibrato[channel_index][MOD_VibratoPosition[channel_index]] *
        (int32_t)MOD_VibratoDepth[channel_index] / 64;
    MOD_VibratoPosition[channel_index] =
        (uint16_t)((MOD_VibratoPosition[channel_index] + MOD_VibratoSpeed[channel_index]) & 63u);
}

static void MOD_MTremolo(uint32_t channel_index)
{
    if (MOD_VibratoSpeed[channel_index] == 0) return;
    MOD_Volume[channel_index] = (uint8_t)MOD_addVolume(
        (int16_t)MOD_TremoloVolume[channel_index],
        (int16_t)((int32_t)MOD_Tremolo[channel_index][MOD_VibratoPosition[channel_index]] *
                  MOD_VibratoDepth[channel_index] / 512));
    MOD_VibratoPosition[channel_index] =
        (uint16_t)((MOD_VibratoPosition[channel_index] + MOD_VibratoSpeed[channel_index]) & 63u);
}

/* ---- retrig ---- */

static void MOD_Retrig(uint32_t channel_index, int8_t count)
{
    MOD_RetrigCount[channel_index]--;
    if (MOD_RetrigCount[channel_index] == 0) {
        MOD_RetrigCount[channel_index]  = count;
        MOD_SamplePosition[channel_index] = 0;
        MOD_SampleFraction[channel_index] = 0;
    }
}

/* ---- global effects (tempo / jump — applied once per row) ---- */

void MOD_GlobalEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        effect = MOD_Effect[channel_index];
        info   = MOD_EffectInfo[channel_index];

        switch (effect) {
        case 0xFFu: break;

        case 11u:  /* B - pattern jump */
            if (info <= MOD_Order) mod_mark_looped();
            MOD_nextorder = info;
            MOD_nextrow   = 0;
            MOD_jump      = 1;
            break;

        case 13u:  /* D - pattern break (BCD) */
            if (MOD_jump) break;
            MOD_nextorder = MOD_Order + 1u;
            MOD_nextrow   = (info >> 4) * 10u + (info & 15u);
            MOD_jump      = 1;
            break;

        case 15u:  /* F - set speed/tempo */
            if (info < 32u) {
                MOD_speed = (uint8_t)info;
            } else {
                MOD_tempo = (uint8_t)info;
                MOD_TickLength = s3m_calc_speed(MOD_tempo, g_MixSpeed);
            }
            break;

        case 14u: {  /* E - extended effects */
            uint32_t x = info >> 4, y = info & 0x0Fu;
            switch (x) {
            case 6u:  /* pattern loop */
                if (y == 0) {
                    MOD_PatLoopRow   = MOD_row;
                    MOD_PatLoopCount = 0;
                } else if (MOD_PatLoopCount == 0) {
                    MOD_PatLoopCount = (int16_t)y;
                    MOD_nextrow      = MOD_PatLoopRow;
                    MOD_nextorder    = MOD_Order;
                    MOD_jump         = 1;
                    MOD_PatLoopCount--;
                } else {
                    MOD_PatLoopCount--;
                    if (MOD_PatLoopCount > 0) {
                        MOD_nextrow   = MOD_PatLoopRow;
                        MOD_nextorder = MOD_Order;
                        MOD_jump      = 1;
                    }
                }
                break;
            case 14u:  /* pattern delay */
                MOD_flag_note = (uint8_t)y;
                break;
            }
            break;
        }
        }
    }
}

/* ---- before-effect (portamento to note preparation) ---- */

void MOD_beforeEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        effect = MOD_Effect[channel_index];
        info   = MOD_EffectInfo[channel_index];
        (void)info;

        switch (effect) {
        case 3u:  /* portamento to note */
        case 5u: {
            uint8_t note = MOD_RowBuffer[channel_index*4+0];
            if (note != 0xFFu && note < 84u) {
                MOD_GlissNote[channel_index] = note;
                MOD_GlissPeriod[channel_index] =
                    MOD_Periods[MOD_SampleFinetune[channel_index]][note] << 4u;
                MOD_GlissFlag[channel_index] = 1;
                MOD_RowBuffer[channel_index*4+0] = 0xFFu;  /* suppress note trigger */
            }
            break;
        }
        case 14u: {  /* extended: note delay */
            uint32_t bx = MOD_EffectInfo[channel_index] >> 4;
            if (bx == 13u) {  /* ED - note delay */
                uint8_t bval;
                MOD_NoteDelayFlag[channel_index] = 0;
                bval = MOD_RowBuffer[channel_index*4+0];
                if (bval != 0xFFu) {
                    MOD_NoteDelayNote[channel_index] = bval;
                    MOD_RowBuffer[channel_index*4+0] = 0xFFu;
                    MOD_NoteDelayFlag[channel_index] |= 1u;
                }
                bval = MOD_RowBuffer[channel_index*4+1];
                if (bval != 0) {
                    MOD_NoteDelayInst[channel_index] = bval;
                    MOD_RowBuffer[channel_index*4+1] = 0;
                    MOD_NoteDelayFlag[channel_index] |= 2u;
                }
            }
            break;
        }
        }
    }
}

/* ---- row effects (applied on row tick) ---- */

void MOD_RowEffect(void)
{
    uint32_t channel_index, effect, info, x, y;

    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        effect = MOD_Effect[channel_index];
        info   = MOD_EffectInfo[channel_index];
        x      = info >> 4;
        y      = info & 0x0Fu;

        switch (effect) {
        case 0xFFu: break;

        case 0u:   /* arpeggio — on row tick use base note period (same formula as tick) */
            if (info) {
                MOD_PeriodAdjust[channel_index] =
                    (int32_t)(MOD_Periods[MOD_SampleFinetune[channel_index]][MOD_Note[channel_index]] << 4u) -
                    (int32_t)MOD_Period[channel_index];
            }
            MOD_VibratoPosition[channel_index] = 0;
            break;

        case 1u:  /* portamento up */
        case 2u:  /* portamento down */
            MOD_PeriodAdjust[channel_index] = 0;
            break;

        case 3u:  /* portamento to note */
            if (info) MOD_GlissSpeed[channel_index] = (uint8_t)info;
            if (MOD_GlissFlag[channel_index] == 1) {
                MOD_GlissFlag[channel_index] = 2;
            }
            if (MOD_GlissPeriod[channel_index] != MOD_Period[channel_index]) {
                MOD_VibratoPosition[channel_index] = 0;
                MOD_PeriodAdjust[channel_index]   = 0;
                MOD_GlissFlag[channel_index]       = 2;
            }
            break;

        case 4u:  /* vibrato */
            if (x) MOD_VibratoSpeed[channel_index] = (uint8_t)x;
            if (y) MOD_VibratoDepth[channel_index] = (uint8_t)y;
            break;

        case 5u:  /* portamento + volume slide */
            MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
            MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
            MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            if (MOD_GlissFlag[channel_index] == 1) {
                MOD_GlissPeriod[channel_index] =
                    MOD_Periods[MOD_SampleFinetune[channel_index]][MOD_GlissNote[channel_index]] << 4u;
                MOD_GlissFlag[channel_index] = 2;
            }
            if (MOD_GlissPeriod[channel_index] != MOD_Period[channel_index]) {
                MOD_VibratoPosition[channel_index] = 0;
                MOD_PeriodAdjust[channel_index]   = 0;
                MOD_GlissFlag[channel_index]       = 2;
            }
            break;

        case 6u:  /* vibrato + volume slide */
            MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
            MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
            MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            break;

        case 7u:  /* tremolo */
            if (x) MOD_VibratoSpeed[channel_index] = (uint8_t)x;
            if (y) MOD_VibratoDepth[channel_index] = (uint8_t)y;
            break;

        case 8u:  /* set panning */
            MOD_Panning[channel_index] = (uint8_t)(info * 100u / 255u);
            break;

        case 9u:  /* sample offset */
            if (info) {
                MOD_SamplePosition[channel_index] = (uint32_t)info << 8;
                MOD_SampleFraction[channel_index] = 0;
            }
            break;

        case 10u: /* volume slide */
            MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
            MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
            MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            break;

        case 12u: /* set volume */
            MOD_Volume[channel_index] = (info > 64u) ? 64u : (uint8_t)info;
            MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            break;

        case 14u: {  /* extended */
            switch (x) {
            case 0u:  /* amiga filter (ignore) */ break;
            case 1u:  /* fine porta up — slide amount is full Info byte */
                MOD_Period[channel_index] = MOD_decPeriod((int32_t)MOD_Period[channel_index], info);
                break;
            case 2u:  /* fine porta down — slide amount is full Info byte */
                MOD_Period[channel_index] = MOD_addPeriod((int32_t)MOD_Period[channel_index], info);
                break;
            case 3u:  /* set gliss control */
                break;
            case 4u:  /* set vibrato waveform */
                if ((y & 3u) == 0) MOD_Vibrato[channel_index] = MOD_Sinus;
                if ((y & 3u) == 1) MOD_Vibrato[channel_index] = MOD_Ramp;
                if ((y & 3u) == 2) MOD_Vibrato[channel_index] = MOD_Square;
                if ((y & 3u) == 3) MOD_Vibrato[channel_index] = MOD_Random;
                break;
            case 5u:  /* set finetune */
                MOD_SampleFinetune[channel_index] = y;
                break;
            case 7u:  /* set tremolo waveform */
                if ((y & 3u) == 0) MOD_Tremolo[channel_index] = MOD_Sinus;
                if ((y & 3u) == 1) MOD_Tremolo[channel_index] = MOD_Ramp;
                if ((y & 3u) == 2) MOD_Tremolo[channel_index] = MOD_Square;
                if ((y & 3u) == 3) MOD_Tremolo[channel_index] = MOD_Random;
                break;
            case 8u:  /* set coarse panning */
                MOD_Panning[channel_index] = (uint8_t)(y * 17u);
                break;
            case 9u:  /* retrig note */
                if (y) MOD_RetrigCount[channel_index] = (int8_t)y;
                break;
            case 10u: /* fine volume up */
                MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
                MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
                break;
            case 11u: /* fine volume down */
                MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
                MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
                break;
            case 12u: /* note cut on tick y */
                break;
            }
            break;
        }
        } /* switch effect */
    }
}

/* ---- tick effects (applied each tick > 0) ---- */

void MOD_TickEffect(void)
{
    uint32_t channel_index, effect, info, x, y;

    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        effect = MOD_Effect[channel_index];
        info   = MOD_EffectInfo[channel_index];
        x      = info >> 4;
        y      = info & 0x0Fu;

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* arpeggio — PeriodAdjust = period(arp_note) - Period */
            if (info) {
                uint32_t note = MOD_Note[channel_index];
                uint32_t phase = MOD_tick % 3u;
                if (phase == 1u) note += x;
                if (phase == 2u) note += y;
                MOD_PeriodAdjust[channel_index] =
                    (int32_t)(Calc_AMIGAperiod(note, MOD_SampleFinetune[channel_index])) -
                    (int32_t)MOD_Period[channel_index];
            }
            break;

        case 1u:  /* portamento up */
            MOD_Period[channel_index] = MOD_decPeriod((int32_t)MOD_Period[channel_index], info << 4);
            break;

        case 2u:  /* portamento down */
            MOD_Period[channel_index] = MOD_addPeriod((int32_t)MOD_Period[channel_index], info << 4);
            break;

        case 3u:  /* portamento to note */
        case 5u:  /* porta + volslide */
            if (effect == 5u) {
                MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
                MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
                MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            }
            if (MOD_GlissFlag[channel_index] == 2) {
                if (MOD_GlissPeriod[channel_index] == MOD_Period[channel_index]) { MOD_GlissFlag[channel_index] = 0; break; }
                if (MOD_GlissPeriod[channel_index] > MOD_Period[channel_index]) {
                    MOD_Period[channel_index] = MOD_addPeriod((int32_t)MOD_Period[channel_index], (uint32_t)MOD_GlissSpeed[channel_index] << 4);
                    if (MOD_Period[channel_index] > MOD_GlissPeriod[channel_index]) MOD_Period[channel_index] = MOD_GlissPeriod[channel_index];
                } else {
                    MOD_Period[channel_index] = MOD_decPeriod((int32_t)MOD_Period[channel_index], (uint32_t)MOD_GlissSpeed[channel_index] << 4);
                    if (MOD_Period[channel_index] < MOD_GlissPeriod[channel_index]) MOD_Period[channel_index] = MOD_GlissPeriod[channel_index];
                }
            }
            break;

        case 4u:  /* vibrato */
        case 6u:  /* vibrato + volslide */
            if (effect == 6u) {
                MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
                MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
                MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            }
            MOD_MVibrato(channel_index);
            break;

        case 7u:  /* tremolo */
            MOD_MTremolo(channel_index);
            break;

        case 9u:  /* sample offset — tick only: retrig */
            break;

        case 10u: /* volume slide */
            MOD_Volume[channel_index] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[channel_index], (int16_t)y);
            MOD_Volume[channel_index] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[channel_index], (int16_t)x);
            MOD_TremoloVolume[channel_index] = MOD_Volume[channel_index];
            break;

        case 14u: {  /* extended tick effects */
            switch (x) {
            case 9u:  /* retrig */
                if (y) MOD_Retrig(channel_index, (int8_t)y);
                break;
            case 12u: /* note cut on tick */
                if (MOD_tick == y) { MOD_Volume[channel_index] = 0; MOD_TremoloVolume[channel_index] = 0; }
                break;
            case 13u: /* note delay — NoteDelayFlag is bitmask: bit1=inst, bit0=note */
                if (MOD_tick == (uint32_t)(MOD_EffectInfo[channel_index] & 0x0Fu)) {
                    if (MOD_NoteDelayFlag[channel_index] & 2u) {
                        MOD_SampleNr[channel_index] = MOD_NoteDelayInst[channel_index];
                        MOD_GetNewSample(channel_index);
                    }
                    if (MOD_NoteDelayFlag[channel_index] & 1u) {
                        MOD_Note[channel_index] = MOD_NoteDelayNote[channel_index];
                        MOD_GetNewNote(channel_index);
                    }
                }
                break;
            }
            break;
        }
        }
    }
}

/* ---- init ---- */

void MOD_initEffects(void)
{
    uint32_t channel_index;
    for (channel_index = 0; channel_index < MOD_MAXCHANNELS; channel_index++) {
        MOD_Vibrato[channel_index]          = MOD_Sinus;
        MOD_Tremolo[channel_index]          = MOD_Sinus;
        MOD_GlissSpeed[channel_index]       = 0;
        MOD_GlissNote[channel_index]        = 0;
        MOD_GlissPeriod[channel_index]     = 0;
        MOD_GlissFlag[channel_index]        = 0;
        MOD_VibratoSpeed[channel_index]     = 0;
        MOD_VibratoDepth[channel_index]     = 0;
        MOD_VibratoPosition[channel_index]  = 0;
        MOD_TremoloVolume[channel_index]    = 0;
        MOD_RetrigCount[channel_index]      = 0;
        MOD_NoteDelayFlag[channel_index]    = 0;
        MOD_NoteDelayNote[channel_index]    = 0;
        MOD_NoteDelayInst[channel_index]    = 0;
    }
    MOD_PatLoopCount = 0;
    MOD_PatLoopRow   = 0;
}

/* ---- read_row: wire effects + note/sample triggers ---- */

void MOD_GetNewEffect(void)
{
    uint32_t channel_index, effect, info;

    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        effect = MOD_Effect[channel_index];
        info   = MOD_EffectInfo[channel_index];
        (void)info;

        /* reset period adjust on new effect */
        switch (effect) {
        case 0xFFu:
        case 0u:
            MOD_PeriodAdjust[channel_index] = 0;
            if (effect == 0u) MOD_VibratoPosition[channel_index] = 0;
            break;
        case 1u:
        case 2u:
            MOD_Period[channel_index] = (uint32_t)((int32_t)MOD_Period[channel_index] + MOD_PeriodAdjust[channel_index]);
            MOD_PeriodAdjust[channel_index] = 0;
            MOD_VibratoPosition[channel_index] = 0;
            break;
        case 3u:
            MOD_VibratoPosition[channel_index] = 0;
            break;
        case 9u:
            MOD_VibratoPosition[channel_index] = 0;
            break;
        case 10u:
            MOD_PeriodAdjust[channel_index] = 0;
            break;
        }
    }

    MOD_GlobalEffect();
    MOD_beforeEffect();
}

void MOD_read_row(void)
{
    uint32_t channel_index;
    uint32_t value;

    /* load effects */
    for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
        value = MOD_RowBuffer[channel_index*4+3];
        if (MOD_EffectInfo[channel_index]) MOD_LastEffectInfo[channel_index] = MOD_EffectInfo[channel_index];
        MOD_EffectInfo[channel_index] = (uint8_t)value;

        value = MOD_RowBuffer[channel_index*4+2];
        if (MOD_Effect[channel_index] != 0xFFu) MOD_LastEffect[channel_index] = MOD_Effect[channel_index];
        MOD_Effect[channel_index] = (uint8_t)value;
    }

    MOD_GetNewEffect();

    /* note / instrument */
    if (MOD_flag_note == 0) {
        for (channel_index = 0; channel_index < MOD_channels; channel_index++) {
            value = MOD_RowBuffer[channel_index*4+1];
            if (value) {
                MOD_SampleNr[channel_index] = (uint8_t)value;
                MOD_GetNewSample(channel_index);
            }

            value = MOD_RowBuffer[channel_index*4+0];
            if (value != 0xFFu) {
                MOD_Note[channel_index] = (uint8_t)value;
                MOD_GetNewNote(channel_index);
                MOD_VibratoPosition[channel_index] = 0;
                MOD_PeriodAdjust[channel_index]   = 0;
            }
        }
    }

    MOD_RowEffect();
}
