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
       uint32_t MOD_GlissPeriode[MOD_MAXCHANNELS];
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

static uint32_t MOD_addPeriode(int32_t p, uint32_t value)
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

static uint32_t MOD_decPeriode(int32_t p, uint32_t value)
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

static void MOD_MVibrato(uint32_t ch)
{
    if (MOD_VibratoSpeed[ch] == 0) return;
    MOD_PeriodeAdjust[ch] =
        (int32_t)MOD_Vibrato[ch][MOD_VibratoPosition[ch]] *
        (int32_t)MOD_VibratoDepth[ch] / 64;
    MOD_VibratoPosition[ch] =
        (uint16_t)((MOD_VibratoPosition[ch] + MOD_VibratoSpeed[ch]) & 63u);
}

static void MOD_MTremolo(uint32_t ch)
{
    if (MOD_VibratoSpeed[ch] == 0) return;
    MOD_Volume[ch] = (uint8_t)MOD_addVolume(
        (int16_t)MOD_TremoloVolume[ch],
        (int16_t)((int32_t)MOD_Tremolo[ch][MOD_VibratoPosition[ch]] *
                  MOD_VibratoDepth[ch] / 512));
    MOD_VibratoPosition[ch] =
        (uint16_t)((MOD_VibratoPosition[ch] + MOD_VibratoSpeed[ch]) & 63u);
}

/* ---- retrig ---- */

static void MOD_Retrig(uint32_t ch, int8_t count)
{
    MOD_RetrigCount[ch]--;
    if (MOD_RetrigCount[ch] == 0) {
        MOD_RetrigCount[ch]  = count;
        MOD_SamplePosition[ch] = 0;
        MOD_SampleFraction[ch] = 0;
    }
}

/* ---- global effects (tempo / jump — applied once per row) ---- */

void MOD_GlobalEffect(void)
{
    uint32_t ch, effect, info;

    for (ch = 0; ch < MOD_channels; ch++) {
        effect = MOD_Effect[ch];
        info   = MOD_EffectInfo[ch];

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
    uint32_t ch, effect, info;

    for (ch = 0; ch < MOD_channels; ch++) {
        effect = MOD_Effect[ch];
        info   = MOD_EffectInfo[ch];
        (void)info;

        switch (effect) {
        case 3u:  /* portamento to note */
        case 5u: {
            uint8_t note = MOD_RowBuffer[ch*4+0];
            if (note != 0xFFu && note < 84u) {
                MOD_GlissNote[ch] = note;
                MOD_GlissPeriode[ch] =
                    MOD_Periodes[MOD_SampleFinetune[ch]][note] << 4u;
                MOD_GlissFlag[ch] = 1;
                MOD_RowBuffer[ch*4+0] = 0xFFu;  /* suppress note trigger */
            }
            break;
        }
        case 14u: {  /* extended: note delay */
            uint32_t bx = MOD_EffectInfo[ch] >> 4;
            if (bx == 13u) {  /* ED - note delay */
                uint8_t bval;
                MOD_NoteDelayFlag[ch] = 0;
                bval = MOD_RowBuffer[ch*4+0];
                if (bval != 0xFFu) {
                    MOD_NoteDelayNote[ch] = bval;
                    MOD_RowBuffer[ch*4+0] = 0xFFu;
                    MOD_NoteDelayFlag[ch] |= 1u;
                }
                bval = MOD_RowBuffer[ch*4+1];
                if (bval != 0) {
                    MOD_NoteDelayInst[ch] = bval;
                    MOD_RowBuffer[ch*4+1] = 0;
                    MOD_NoteDelayFlag[ch] |= 2u;
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
    uint32_t ch, effect, info, x, y;

    for (ch = 0; ch < MOD_channels; ch++) {
        effect = MOD_Effect[ch];
        info   = MOD_EffectInfo[ch];
        x      = info >> 4;
        y      = info & 0x0Fu;

        switch (effect) {
        case 0xFFu: break;

        case 0u:   /* arpeggio — on row tick use base note period (same formula as tick) */
            if (info) {
                MOD_PeriodeAdjust[ch] =
                    (int32_t)(MOD_Periodes[MOD_SampleFinetune[ch]][MOD_Note[ch]] << 4u) -
                    (int32_t)MOD_Periode[ch];
            }
            MOD_VibratoPosition[ch] = 0;
            break;

        case 1u:  /* portamento up */
        case 2u:  /* portamento down */
            MOD_PeriodeAdjust[ch] = 0;
            break;

        case 3u:  /* portamento to note */
            if (info) MOD_GlissSpeed[ch] = (uint8_t)info;
            if (MOD_GlissFlag[ch] == 1) {
                MOD_GlissFlag[ch] = 2;
            }
            if (MOD_GlissPeriode[ch] != MOD_Periode[ch]) {
                MOD_VibratoPosition[ch] = 0;
                MOD_PeriodeAdjust[ch]   = 0;
                MOD_GlissFlag[ch]       = 2;
            }
            break;

        case 4u:  /* vibrato */
            if (x) MOD_VibratoSpeed[ch] = (uint8_t)x;
            if (y) MOD_VibratoDepth[ch] = (uint8_t)y;
            break;

        case 5u:  /* portamento + volume slide */
            MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
            MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
            MOD_TremoloVolume[ch] = MOD_Volume[ch];
            if (MOD_GlissFlag[ch] == 1) {
                MOD_GlissPeriode[ch] =
                    MOD_Periodes[MOD_SampleFinetune[ch]][MOD_GlissNote[ch]] << 4u;
                MOD_GlissFlag[ch] = 2;
            }
            if (MOD_GlissPeriode[ch] != MOD_Periode[ch]) {
                MOD_VibratoPosition[ch] = 0;
                MOD_PeriodeAdjust[ch]   = 0;
                MOD_GlissFlag[ch]       = 2;
            }
            break;

        case 6u:  /* vibrato + volume slide */
            MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
            MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
            MOD_TremoloVolume[ch] = MOD_Volume[ch];
            break;

        case 7u:  /* tremolo */
            if (x) MOD_VibratoSpeed[ch] = (uint8_t)x;
            if (y) MOD_VibratoDepth[ch] = (uint8_t)y;
            break;

        case 8u:  /* set panning */
            MOD_Panning[ch] = (uint8_t)(info * 100u / 255u);
            break;

        case 9u:  /* sample offset */
            if (info) {
                MOD_SamplePosition[ch] = (uint32_t)info << 8;
                MOD_SampleFraction[ch] = 0;
            }
            break;

        case 10u: /* volume slide */
            MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
            MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
            MOD_TremoloVolume[ch] = MOD_Volume[ch];
            break;

        case 12u: /* set volume */
            MOD_Volume[ch] = (info > 64u) ? 64u : (uint8_t)info;
            MOD_TremoloVolume[ch] = MOD_Volume[ch];
            break;

        case 14u: {  /* extended */
            switch (x) {
            case 0u:  /* amiga filter (ignore) */ break;
            case 1u:  /* fine porta up — slide amount is full Info byte */
                MOD_Periode[ch] = MOD_decPeriode((int32_t)MOD_Periode[ch], info);
                break;
            case 2u:  /* fine porta down — slide amount is full Info byte */
                MOD_Periode[ch] = MOD_addPeriode((int32_t)MOD_Periode[ch], info);
                break;
            case 3u:  /* set gliss control */
                break;
            case 4u:  /* set vibrato waveform */
                if ((y & 3u) == 0) MOD_Vibrato[ch] = MOD_Sinus;
                if ((y & 3u) == 1) MOD_Vibrato[ch] = MOD_Ramp;
                if ((y & 3u) == 2) MOD_Vibrato[ch] = MOD_Square;
                if ((y & 3u) == 3) MOD_Vibrato[ch] = MOD_Random;
                break;
            case 5u:  /* set finetune */
                MOD_SampleFinetune[ch] = y;
                break;
            case 7u:  /* set tremolo waveform */
                if ((y & 3u) == 0) MOD_Tremolo[ch] = MOD_Sinus;
                if ((y & 3u) == 1) MOD_Tremolo[ch] = MOD_Ramp;
                if ((y & 3u) == 2) MOD_Tremolo[ch] = MOD_Square;
                if ((y & 3u) == 3) MOD_Tremolo[ch] = MOD_Random;
                break;
            case 8u:  /* set coarse panning */
                MOD_Panning[ch] = (uint8_t)(y * 17u);
                break;
            case 9u:  /* retrig note */
                if (y) MOD_RetrigCount[ch] = (int8_t)y;
                break;
            case 10u: /* fine volume up */
                MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)y);
                MOD_TremoloVolume[ch] = MOD_Volume[ch];
                break;
            case 11u: /* fine volume down */
                MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
                MOD_TremoloVolume[ch] = MOD_Volume[ch];
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
    uint32_t ch, effect, info, x, y;

    for (ch = 0; ch < MOD_channels; ch++) {
        effect = MOD_Effect[ch];
        info   = MOD_EffectInfo[ch];
        x      = info >> 4;
        y      = info & 0x0Fu;

        switch (effect) {
        case 0xFFu: break;

        case 0u:  /* arpeggio — PeriodeAdjust = period(arp_note) - Periode */
            if (info) {
                uint32_t note = MOD_Note[ch];
                uint32_t phase = MOD_tick % 3u;
                if (phase == 1u) note += x;
                if (phase == 2u) note += y;
                MOD_PeriodeAdjust[ch] =
                    (int32_t)(Calc_AMIGAperiode(note, MOD_SampleFinetune[ch])) -
                    (int32_t)MOD_Periode[ch];
            }
            break;

        case 1u:  /* portamento up */
            MOD_Periode[ch] = MOD_decPeriode((int32_t)MOD_Periode[ch], info << 4);
            break;

        case 2u:  /* portamento down */
            MOD_Periode[ch] = MOD_addPeriode((int32_t)MOD_Periode[ch], info << 4);
            break;

        case 3u:  /* portamento to note */
        case 5u:  /* porta + volslide */
            if (effect == 5u) {
                MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
                MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
                MOD_TremoloVolume[ch] = MOD_Volume[ch];
            }
            if (MOD_GlissFlag[ch] == 2) {
                if (MOD_GlissPeriode[ch] == MOD_Periode[ch]) { MOD_GlissFlag[ch] = 0; break; }
                if (MOD_GlissPeriode[ch] > MOD_Periode[ch]) {
                    MOD_Periode[ch] = MOD_addPeriode((int32_t)MOD_Periode[ch], (uint32_t)MOD_GlissSpeed[ch] << 4);
                    if (MOD_Periode[ch] > MOD_GlissPeriode[ch]) MOD_Periode[ch] = MOD_GlissPeriode[ch];
                } else {
                    MOD_Periode[ch] = MOD_decPeriode((int32_t)MOD_Periode[ch], (uint32_t)MOD_GlissSpeed[ch] << 4);
                    if (MOD_Periode[ch] < MOD_GlissPeriode[ch]) MOD_Periode[ch] = MOD_GlissPeriode[ch];
                }
            }
            break;

        case 4u:  /* vibrato */
        case 6u:  /* vibrato + volslide */
            if (effect == 6u) {
                MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
                MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
                MOD_TremoloVolume[ch] = MOD_Volume[ch];
            }
            MOD_MVibrato(ch);
            break;

        case 7u:  /* tremolo */
            MOD_MTremolo(ch);
            break;

        case 9u:  /* sample offset — tick only: retrig */
            break;

        case 10u: /* volume slide */
            MOD_Volume[ch] = (uint8_t)MOD_decVolume((int16_t)MOD_Volume[ch], (int16_t)y);
            MOD_Volume[ch] = (uint8_t)MOD_addVolume((int16_t)MOD_Volume[ch], (int16_t)x);
            MOD_TremoloVolume[ch] = MOD_Volume[ch];
            break;

        case 14u: {  /* extended tick effects */
            switch (x) {
            case 9u:  /* retrig */
                if (y) MOD_Retrig(ch, (int8_t)y);
                break;
            case 12u: /* note cut on tick */
                if (MOD_tick == y) { MOD_Volume[ch] = 0; MOD_TremoloVolume[ch] = 0; }
                break;
            case 13u: /* note delay — NoteDelayFlag is bitmask: bit1=inst, bit0=note */
                if (MOD_tick == (uint32_t)(MOD_EffectInfo[ch] & 0x0Fu)) {
                    if (MOD_NoteDelayFlag[ch] & 2u) {
                        MOD_SampleNr[ch] = MOD_NoteDelayInst[ch];
                        MOD_GetNewSample(ch);
                    }
                    if (MOD_NoteDelayFlag[ch] & 1u) {
                        MOD_Note[ch] = MOD_NoteDelayNote[ch];
                        MOD_GetNewNote(ch);
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
    uint32_t ch;
    for (ch = 0; ch < MOD_MAXCHANNELS; ch++) {
        MOD_Vibrato[ch]          = MOD_Sinus;
        MOD_Tremolo[ch]          = MOD_Sinus;
        MOD_GlissSpeed[ch]       = 0;
        MOD_GlissNote[ch]        = 0;
        MOD_GlissPeriode[ch]     = 0;
        MOD_GlissFlag[ch]        = 0;
        MOD_VibratoSpeed[ch]     = 0;
        MOD_VibratoDepth[ch]     = 0;
        MOD_VibratoPosition[ch]  = 0;
        MOD_TremoloVolume[ch]    = 0;
        MOD_RetrigCount[ch]      = 0;
        MOD_NoteDelayFlag[ch]    = 0;
        MOD_NoteDelayNote[ch]    = 0;
        MOD_NoteDelayInst[ch]    = 0;
    }
    MOD_PatLoopCount = 0;
    MOD_PatLoopRow   = 0;
}

/* ---- read_row: wire effects + note/sample triggers ---- */

void MOD_GetNewEffect(void)
{
    uint32_t ch, effect, info;

    for (ch = 0; ch < MOD_channels; ch++) {
        effect = MOD_Effect[ch];
        info   = MOD_EffectInfo[ch];
        (void)info;

        /* reset period adjust on new effect */
        switch (effect) {
        case 0xFFu:
        case 0u:
            MOD_PeriodeAdjust[ch] = 0;
            if (effect == 0u) MOD_VibratoPosition[ch] = 0;
            break;
        case 1u:
        case 2u:
            MOD_Periode[ch] = (uint32_t)((int32_t)MOD_Periode[ch] + MOD_PeriodeAdjust[ch]);
            MOD_PeriodeAdjust[ch] = 0;
            MOD_VibratoPosition[ch] = 0;
            break;
        case 3u:
            MOD_VibratoPosition[ch] = 0;
            break;
        case 9u:
            MOD_VibratoPosition[ch] = 0;
            break;
        case 10u:
            MOD_PeriodeAdjust[ch] = 0;
            break;
        }
    }

    MOD_GlobalEffect();
    MOD_beforeEffect();
}

void MOD_read_row(void)
{
    uint32_t ch;
    uint32_t value;

    /* load effects */
    for (ch = 0; ch < MOD_channels; ch++) {
        value = MOD_RowBuffer[ch*4+3];
        if (MOD_EffectInfo[ch]) MOD_LastEffectInfo[ch] = MOD_EffectInfo[ch];
        MOD_EffectInfo[ch] = (uint8_t)value;

        value = MOD_RowBuffer[ch*4+2];
        if (MOD_Effect[ch] != 0xFFu) MOD_LastEffect[ch] = MOD_Effect[ch];
        MOD_Effect[ch] = (uint8_t)value;
    }

    MOD_GetNewEffect();

    /* note / instrument */
    if (MOD_flag_note == 0) {
        for (ch = 0; ch < MOD_channels; ch++) {
            value = MOD_RowBuffer[ch*4+1];
            if (value) {
                MOD_SampleNr[ch] = (uint8_t)value;
                MOD_GetNewSample(ch);
            }

            value = MOD_RowBuffer[ch*4+0];
            if (value != 0xFFu) {
                MOD_Note[ch] = (uint8_t)value;
                MOD_GetNewNote(ch);
                MOD_VibratoPosition[ch] = 0;
                MOD_PeriodeAdjust[ch]   = 0;
            }
        }
    }

    MOD_RowEffect();
}
