#include <stdint.h>
#include "effect/efc_s3m.h"
#include "format/s3m.h"
#include "util/calc.h"
#include "mixer/mixer_scalar.h"

/* ---- exported effect state ---- */

uint32_t S3M_GlissPeriode[S3M_MAXCHANNELS];
uint16_t S3M_VibratoPosition[S3M_MAXCHANNELS];
uint16_t S3M_TremorVolume[S3M_MAXCHANNELS];

/* ---- waveform tables ---- */

static const int16_t Sinus[64] = {
       0,  100,  200,  297,  392,  483,  569,  650,
     724,  792,  851,  903,  946,  980, 1004, 1019,
    1024, 1019, 1004,  980,  946,  903,  851,  792,
     724,  650,  569,  483,  392,  297,  200,  100,
       0, -100, -200, -297, -392, -483, -569, -650,
    -724, -792, -851, -903, -946, -980,-1004,-1019,
   -1024,-1019,-1004, -980, -946, -903, -851, -792,
    -724, -650, -569, -483, -392, -297, -200, -100
};

static const int16_t Ramp[64] = {
   -1024, -992, -960, -928, -896, -864, -832, -800,
    -768, -736, -704, -672, -640, -608, -576, -544,
    -512, -480, -448, -416, -384, -352, -320, -288,
    -256, -224, -192, -160, -128,  -96,  -64,  -32,
       0,   32,   64,   96,  128,  160,  192,  224,
     256,  288,  320,  352,  384,  416,  448,  480,
     512,  544,  576,  608,  640,  672,  704,  736,
     768,  800,  832,  864,  896,  928,  960,  992
};

static const int16_t Square[64] = {
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int16_t Random[64] = {
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

static const int16_t *Vibrato[S3M_MAXCHANNELS];
static const int16_t *Tremolo[S3M_MAXCHANNELS];

static uint8_t  GlissSpeed[S3M_MAXCHANNELS];
static uint8_t  GlissNote[S3M_MAXCHANNELS];
static uint8_t  GlissOctave[S3M_MAXCHANNELS];
static uint8_t  GlissFlag[S3M_MAXCHANNELS];

static uint8_t  VibratoSpeed[S3M_MAXCHANNELS];
static uint8_t  VibratoDepth[S3M_MAXCHANNELS];

static uint8_t  TremorCountOn[S3M_MAXCHANNELS];
static uint8_t  TremorCountOff[S3M_MAXCHANNELS];

static int8_t   S3M_RetrigCount[S3M_MAXCHANNELS];

static uint8_t  NoteDelayFlag[S3M_MAXCHANNELS];
static uint8_t  NoteDelayNote[S3M_MAXCHANNELS];
static uint8_t  NoteDelayInst[S3M_MAXCHANNELS];
static uint8_t  NoteDelayVol[S3M_MAXCHANNELS];

static int32_t  PatLoopCount = 0;

/* ---- volume / period helpers ---- */

static uint8_t S3M_addVolume(int16_t vol, int16_t val)
{
    vol += val;
    if (vol > 64) vol = 64;
    else if (vol < 0) vol = 0;
    return (uint8_t)vol;
}

static uint8_t S3M_decVolume(int16_t vol, int16_t val)
{
    vol -= val;
    if (vol < 0) vol = 0;
    else if (vol > 64) vol = 64;
    return (uint8_t)vol;
}

static uint32_t S3M_addPeriode(int32_t periode, uint32_t val)
{
    periode += (int32_t)val;
    if (S3M_flag_amigalimits) {
        if (periode < 7232)  periode = 7232;
        if (periode > 54784) periode = 54784;
    } else {
        if (periode < 59) periode = 59;
    }
    return (uint32_t)periode;
}

static uint32_t S3M_decPeriode(int32_t periode, uint32_t val)
{
    periode -= (int32_t)val;
    if (S3M_flag_amigalimits) {
        if (periode < 7232)  periode = 7232;
        if (periode > 54784) periode = 54784;
    } else {
        if (periode < 59) periode = 59;
    }
    return (uint32_t)periode;
}

/* ---- vibrato / tremor / retrig helpers ---- */

static void S3M_Vibrato(uint32_t channel)
{
    if (VibratoSpeed[channel] == 0) return;
    S3M_PeriodeAdjust[channel] =
        Vibrato[channel][S3M_VibratoPosition[channel]] * VibratoDepth[channel] / 8;
    S3M_VibratoPosition[channel] =
        (uint16_t)((S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63u);
}

static void S3M_FineVibrato(uint32_t channel)
{
    if (VibratoSpeed[channel] == 0) return;
    S3M_PeriodeAdjust[channel] =
        Vibrato[channel][S3M_VibratoPosition[channel]] * VibratoDepth[channel] / 64;
    S3M_VibratoPosition[channel] =
        (uint16_t)((S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63u);
}

static void S3M_Tremor(uint32_t channel, uint8_t count_on, uint8_t count_off)
{
    if (TremorCountOn[channel] != 0) {
        S3M_Volume[channel] = (uint8_t)S3M_TremorVolume[channel];
        TremorCountOn[channel]--;
        TremorCountOff[channel] = count_off;
    } else {
        S3M_Volume[channel] = 0;
        TremorCountOff[channel]--;
        if (TremorCountOff[channel] == 0) TremorCountOn[channel] = count_on;
    }
}

static void S3M_Retrig(uint32_t channel, uint8_t vol, int8_t count)
{
    S3M_RetrigCount[channel]--;
    if (S3M_RetrigCount[channel] != 0) return;
    S3M_RetrigCount[channel] = count;
    S3M_SamplePosition[channel] = 0;
    S3M_SampleFraction[channel] = 0;
    switch (vol) {
        case  0: break;
        case  1: S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 1);  break;
        case  2: S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 2);  break;
        case  3: S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 4);  break;
        case  4: S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 8);  break;
        case  5: S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 16); break;
        case  6:
            S3M_Volume[channel] = (uint8_t)((uint16_t)S3M_Volume[channel] * 625u / 1000u);
            break;
        case  7: S3M_Volume[channel] >>= 1; break;
        case  8: break;
        case  9: S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 1);  break;
        case 10: S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 2);  break;
        case 11: S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 4);  break;
        case 12: S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 8);  break;
        case 13: S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 16); break;
        case 14:
            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],
                                                (int16_t)(S3M_Volume[channel] >> 1));
            break;
        case 15:
            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],
                                                (int16_t)S3M_Volume[channel]);
            break;
        default: break;
    }
}

static void S3M_Tremolo(uint32_t channel)
{
    if (VibratoSpeed[channel] == 0) return;
    S3M_Volume[channel] = S3M_addVolume(
        (int16_t)S3M_TremorVolume[channel],
        (int16_t)(Tremolo[channel][S3M_VibratoPosition[channel]]
                  * VibratoDepth[channel] / 512));
    S3M_VibratoPosition[channel] =
        (uint16_t)((S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63u);
}

/* ---- public effect functions ---- */

void S3M_initEffects(void)
{
    uint32_t channel;
    for (channel = 0; channel < S3M_MAXCHANNELS; channel++) {
        Vibrato[channel] = Sinus;
        Tremolo[channel] = Sinus;
        GlissSpeed[channel]        = 0;
        GlissNote[channel]         = 0;
        GlissOctave[channel]       = 0;
        S3M_GlissPeriode[channel]  = 0;
        GlissFlag[channel]         = 0;
        VibratoSpeed[channel]      = 0;
        VibratoDepth[channel]      = 0;
        S3M_VibratoPosition[channel] = 0;
        S3M_TremorVolume[channel]  = 0;
        TremorCountOn[channel]     = 0;
        TremorCountOff[channel]    = 0;
        S3M_RetrigCount[channel]   = 0;
        NoteDelayFlag[channel]     = 0;
        NoteDelayNote[channel]     = 0;
        NoteDelayInst[channel]     = 0;
        NoteDelayVol[channel]      = 0;
    }
    PatLoopCount = 0;
}

void S3M_GlobalEffect(void)
{
    uint32_t channel;
    uint8_t  effect, info;
    uint8_t  x, y;
    int16_t  jumpbreak = 0, special_tempo = 0, special_global = 0;

    for (channel = 0; channel < S3M_channels; channel++) {
        effect = S3M_Effect[channel];
        info   = S3M_EffectInfo[channel];
        x = info >> 4;
        y = info & 15u;

        switch (effect) {
            case 255: break;

            case 1:  /* Axx: set speed */
                if (info == 0) break;
                S3M_speed = info;
                break;

            case 2:  /* Bxx: jump to order */
                if (jumpbreak) break;
                S3M_nextorder = info;
                S3M_nextrow   = 0;
                S3M_jump      = 1;
                jumpbreak     = 1;
                break;

            case 3:  /* Cxx: break pattern to row (BCD) */
                if (jumpbreak) break;
                info = (uint8_t)((info >> 4) * 10u + (info & 15u));
                S3M_nextorder = S3M_Order + 1u;
                S3M_nextrow   = info;
                S3M_jump      = 1;
                jumpbreak     = 1;
                break;

            case 19:  /* Sxy: special */
                switch (x) {
                    case 0x03:  /* set vibrato waveform */
                        if ((y & 3u) == 0) Vibrato[channel] = Sinus;
                        if ((y & 3u) == 1) Vibrato[channel] = Ramp;
                        if ((y & 3u) == 2) Vibrato[channel] = Square;
                        if ((y & 3u) == 3) Vibrato[channel] = Random;
                        break;
                    case 0x04:  /* set tremolo waveform */
                        if ((y & 3u) == 0) Tremolo[channel] = Sinus;
                        if ((y & 3u) == 1) Tremolo[channel] = Ramp;
                        if ((y & 3u) == 2) Tremolo[channel] = Square;
                        if ((y & 3u) == 3) Tremolo[channel] = Random;
                        break;
                    case 0x08:  /* set pan position */
                        S3M_Panning[channel] = y;
                        break;
                    case 0x0a:  /* old panning */
                        switch (y) {
                            case 0: S3M_Panning[channel] = 3;  break;
                            case 1: S3M_Panning[channel] = 12; break;
                            case 2: S3M_Panning[channel] = 0;  break;
                            case 3: S3M_Panning[channel] = 15; break;
                            case 4: S3M_Panning[channel] = 5;  break;
                            case 5: S3M_Panning[channel] = 10; break;
                            case 6: S3M_Panning[channel] = 7;  break;
                            case 7: S3M_Panning[channel] = 8;  break;
                            default: break;
                        }
                        break;
                    case 0x0b:  /* SBx pattern loop */
                        if (y == 0) {
                            S3M_nextrow = S3M_row;
                        } else {
                            if (PatLoopCount == 0) PatLoopCount = y + 1;
                            PatLoopCount--;
                            if (PatLoopCount != 0) {
                                S3M_nextorder = S3M_Order;
                                S3M_jump      = 1;
                            } else {
                                S3M_nextrow = S3M_row + 1u;
                                S3M_jump    = 0;
                            }
                        }
                        break;
                    default: break;
                }
                break;

            case 20:  /* Txx: set tempo */
                if (special_tempo) break;
                if (info < 0x21) break;
                S3M_tempo      = info;
                S3M_TickLength = s3m_calc_speed(S3M_tempo, g_MixSpeed);
                special_tempo  = 1;
                break;

            case 22:  /* Vxx: set global volume */
                if (special_global) break;
                if (info > 0x40) break;
                S3M_GlobalVolume = info;
                special_global   = 1;
                break;

            default: break;
        }
    }
}

void S3M_beforeEffect(void)
{
    uint32_t channel;
    uint8_t  effect, info, x, y, value;

    for (channel = 0; channel < S3M_channels; channel++) {
        effect = S3M_Effect[channel];
        info   = S3M_EffectInfo[channel];
        x = info >> 4;
        y = info & 15u;

        switch (effect) {
            case 255: break;

            case 7:  /* Gxx: tone portamento — capture target note */
                value = S3M_RowBuffer[channel * 5 + 0];
                if (value != 255 && value != 254) {
                    S3M_PeriodeAdjust[channel] = 0;
                    GlissNote[channel]   = value & 15u;
                    GlissOctave[channel] = value >> 4;
                    if (GlissNote[channel] < 13 && GlissOctave[channel] < 8) {
                        S3M_RowBuffer[channel * 5 + 0] = 255;
                        GlissFlag[channel] = 1;
                    } else {
                        GlissFlag[channel] = 0;
                    }
                }
                break;

            case 12:  /* Lxy */
                value = S3M_RowBuffer[channel * 5 + 0];
                if (value != 255 && value != 254) {
                    S3M_PeriodeAdjust[channel] = 0;
                    S3M_RowBuffer[channel * 5 + 0] = 255;
                }
                break;

            case 19:  /* Sxy: note delay */
                if (x == 0x0d) {
                    NoteDelayFlag[channel] = 0;
                    value = S3M_RowBuffer[channel * 5 + 0];
                    if (value != 255 && value != 254) {
                        NoteDelayNote[channel] = value;
                        S3M_RowBuffer[channel * 5 + 0] = 255;
                        NoteDelayFlag[channel] |= 1u;
                    }
                    value = S3M_RowBuffer[channel * 5 + 1];
                    if (value != 0) {
                        NoteDelayInst[channel] = value;
                        S3M_RowBuffer[channel * 5 + 1] = 0;
                        NoteDelayFlag[channel] |= 2u;
                    }
                    value = S3M_RowBuffer[channel * 5 + 2];
                    if (value != 255) {
                        NoteDelayVol[channel] = value;
                        S3M_RowBuffer[channel * 5 + 2] = 255;
                        NoteDelayFlag[channel] |= 4u;
                    }
                }
                break;

            default: break;
        }
        (void)y;  /* suppress unused warning for y in cases that don't use it */
    }
}

void S3M_RowEffect(void)
{
    uint32_t channel;
    uint8_t  effect, info, x, y, value;

    for (channel = 0; channel < S3M_channels; channel++) {
        effect = S3M_Effect[channel];
        info   = S3M_EffectInfo[channel];
        x = info >> 4;
        y = info & 15u;

        switch (effect) {
            case 255: break;

            case 4:  /* Dxy: volume slide */
                if (x == 0) {
                    if (y == 0x0f || S3M_flag_st3volslides)
                        S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
                } else if (y == 0) {
                    if (x == 0x0f || S3M_flag_st3volslides)
                        S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
                } else if (x == 0x0f && y != 0 && y != 0x0f) {
                    S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
                } else if (y == 0x0f && x != 0 && x != 0x0f) {
                    S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
                } else if (y == 0x0f && x == 0x0f) {
                    S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 0x0f);
                }
                break;

            case 5:  /* Exx: slide down */
                if (x == 0x0f)
                    S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel], y * 64u);
                else if (x == 0x0e)
                    S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel], y * 16u);
                break;

            case 6:  /* Fxx: slide up */
                if (x == 0x0f)
                    S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel], y * 64u);
                else if (x == 0x0e)
                    S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel], y * 16u);
                break;

            case 7:  /* Gxx: tone portamento */
                if (info != 0) GlissSpeed[channel] = info;
                if (GlissFlag[channel] == 1) {
                    uint32_t dum = (uint32_t)(8363u * 16u *
                        ((uint32_t)(S3M_NotePeriodes[GlissNote[channel]] << 4)
                         >> GlissOctave[channel]));
                    S3M_GlissPeriode[channel] = dum / S3M_SampleC4SPD[channel];
                    GlissFlag[channel] = 2;
                }
                if (S3M_GlissPeriode[channel] != S3M_Periode[channel]) {
                    S3M_VibratoPosition[channel] = 0;
                    S3M_PeriodeAdjust[channel]   = 0;
                    GlissFlag[channel] = 2;
                }
                break;

            case 8:  /* Hxy: vibrato */
                if (x) VibratoSpeed[channel] = x;
                if (y) VibratoDepth[channel] = y;
                break;

            case 9:  /* Ixy: tremor */
                if (TremorCountOff[channel] == 0) TremorCountOn[channel] = (uint8_t)(x + 1u);
                S3M_Tremor(channel, (uint8_t)(x + 1u), (uint8_t)(y + 1u));
                break;

            case 10:  /* Jxy: arpeggio (row tick = base note) */
                if (S3M_Note[channel] <= 11 && S3M_SampleC4SPD[channel] != 0)
                    S3M_PeriodeAdjust[channel] =
                        (int32_t)Calc_st3periode(S3M_Note[channel], S3M_Octave[channel],
                                                 S3M_SampleC4SPD[channel])
                        - (int32_t)S3M_Periode[channel];
                break;

            case 11: case 12: break;

            case 15:  /* Oxx: sample offset */
                value = S3M_RowBuffer[channel * 5 + 0];
                if (value != 255 && value != 254) {
                    S3M_SamplePosition[channel] = (uint32_t)info << 8;
                    S3M_SampleFraction[channel] = 0;
                }
                break;

            case 17:  /* Qxy: retrig */
                if (S3M_LastEffect[channel] == 17)
                    S3M_Retrig(channel, x, (int8_t)y);
                else if (info != 0)
                    S3M_RetrigCount[channel] = (int8_t)y;
                break;

            case 18:  /* Rxy: tremolo */
                if (x) VibratoSpeed[channel] = x;
                if (y) VibratoDepth[channel] = y;
                break;

            case 19:  /* Sxy: pattern delay */
                if (x == 0x0e) {
                    if (y != 0) {
                        y--;
                        S3M_RowBuffer[channel * 5 + 4] = (uint8_t)(0xe0u + y);
                        S3M_flag_note = 1;
                    } else {
                        S3M_flag_note = 0;
                    }
                }
                break;

            case 21:  /* Uxy: fine vibrato */
                if (x) VibratoSpeed[channel] = x;
                if (y) VibratoDepth[channel] = y;
                S3M_FineVibrato(channel);
                break;

            default: break;
        }
    }
}

void S3M_TickEffect(void)
{
    uint32_t channel;
    uint8_t  effect, info, x, y;

    for (channel = 0; channel < S3M_channels; channel++) {
        effect = S3M_Effect[channel];
        info   = S3M_EffectInfo[channel];
        x = info >> 4;
        y = info & 15u;

        switch (effect) {
            case 255: break;

            case 4:  /* Dxy: volume slide */
                if (x == 0)
                    S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
                else if (y == 0)
                    S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
                break;

            case 5:  /* Exx: slide down */
                if (x != 0x0f && x != 0x0e)
                    S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],
                                                         (uint32_t)info * 64u);
                break;

            case 6:  /* Fxx: slide up */
                if (x != 0x0f && x != 0x0e)
                    S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],
                                                         (uint32_t)info * 64u);
                break;

            case 7:  /* Gxx: tone portamento */
                if (S3M_GlissPeriode[channel] == S3M_Periode[channel]
                        || GlissFlag[channel] != 2) {
                    GlissFlag[channel] = 0;
                    break;
                }
                if (S3M_GlissPeriode[channel] > S3M_Periode[channel]) {
                    S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],
                                                         GlissSpeed[channel] * 64u);
                    if (S3M_Periode[channel] > S3M_GlissPeriode[channel])
                        S3M_Periode[channel] = S3M_GlissPeriode[channel];
                } else {
                    S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],
                                                         GlissSpeed[channel] * 64u);
                    if (S3M_Periode[channel] < S3M_GlissPeriode[channel])
                        S3M_Periode[channel] = S3M_GlissPeriode[channel];
                }
                break;

            case 8:  /* Hxy: vibrato */
                S3M_Vibrato(channel);
                break;

            case 9:  /* Ixy: tremor */
                S3M_Tremor(channel, (uint8_t)(x + 1u), (uint8_t)(y + 1u));
                break;

            case 10: { /* Jxy: arpeggio */
                uint8_t note = S3M_Note[channel];
                uint8_t oct  = S3M_Octave[channel];
                if ((S3M_tick % 3u) == 1u) note += x;
                if ((S3M_tick % 3u) == 2u) note += y;
                while (note > 11) { note -= 12; oct++; }
                if (note > 11 || oct > 8) break;  /* guard out-of-range */
                S3M_PeriodeAdjust[channel] =
                    (int32_t)Calc_st3periode(note, oct, S3M_SampleC4SPD[channel])
                    - (int32_t)S3M_Periode[channel];
                break;
            }

            case 11:  /* Kxy: vibrato + volume slide */
                if ((x == 0x0f && y != 0) || (y == 0x0f && x != 0)) break;
                if (x == 0)
                    S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
                else if (y == 0)
                    S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
                S3M_Vibrato(channel);
                break;

            case 12:  /* Lxy: portamento + volume slide */
                if ((x == 0x0f && y != 0) || (y == 0x0f && x != 0)) break;
                if (x == 0)
                    S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
                else if (y == 0)
                    S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
                if (S3M_GlissPeriode[channel] == S3M_Periode[channel]
                        || GlissFlag[channel] != 2) {
                    GlissFlag[channel] = 0;
                    break;
                }
                if (S3M_GlissPeriode[channel] > S3M_Periode[channel]) {
                    S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],
                                                         GlissSpeed[channel] * 64u);
                    if (S3M_Periode[channel] > S3M_GlissPeriode[channel])
                        S3M_Periode[channel] = S3M_GlissPeriode[channel];
                } else {
                    S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],
                                                         GlissSpeed[channel] * 64u);
                    if (S3M_Periode[channel] < S3M_GlissPeriode[channel])
                        S3M_Periode[channel] = S3M_GlissPeriode[channel];
                }
                break;

            case 17:  /* Qxy: retrig */
                S3M_Retrig(channel, x, (int8_t)y);
                break;

            case 18:  /* Rxy: tremolo */
                S3M_Tremolo(channel);
                break;

            case 19:  /* Sxy: note cut / note delay */
                switch (x) {
                    case 0x0c:  /* note cut */
                        if (S3M_tick == y) S3M_ChannelActiv[channel] = 0;
                        break;
                    case 0x0d:  /* note delay */
                        if (S3M_tick == y) {
                            if (NoteDelayFlag[channel] & 2u) {
                                S3M_SampleNr[channel] = NoteDelayInst[channel];
                                S3M_GetNewSample(channel);
                            }
                            if (NoteDelayFlag[channel] & 4u)
                                S3M_Volume[channel] =
                                    (uint8_t)((S3M_GlobalVolume * NoteDelayVol[channel] + 32u) / 64u);
                            if (NoteDelayFlag[channel] & 1u) {
                                S3M_Note[channel]   = NoteDelayNote[channel] & 15u;
                                S3M_Octave[channel] = NoteDelayNote[channel] >> 4;
                                S3M_GetNewNote(channel);
                            }
                        }
                        break;
                    default: break;
                }
                break;

            case 21:  /* Uxy: fine vibrato */
                S3M_FineVibrato(channel);
                break;

            default: break;
        }
    }
}
