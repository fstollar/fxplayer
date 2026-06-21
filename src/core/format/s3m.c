#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "format/s3m.h"
#include "effect/efc_s3m.h"
#include "util/calc.h"
#include "mixer/mixer_scalar.h"

/* ---- globals ---- */

uint32_t S3M_NotePeriodes[12] =
    { 1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 907 };

static uint32_t S3M_PanningTable[16] =
    { 0, 18, 37, 55, 73, 91, 110, 128, 144, 160, 176, 192, 208, 224, 240, 256 };

uint32_t  S3M_OrderNum = 0, S3M_InstrumentNum = 0, S3M_PatternNum = 0;
uint32_t  S3M_channels = S3M_MAXCHANNELS;
uint8_t   S3M_RowBuffer[S3M_MAXCHANNELS * 5];

uint32_t  S3M_row = 0, S3M_jump = 0, S3M_nextrow = 0, S3M_nextorder = 0;
uint32_t  S3M_tick = 0, S3M_Order = 0, S3M_Pattern = 0;
uint32_t  S3M_TickLength = 0;
uint32_t  S3M_buffer_rest = 0, S3M_tick_rest = 0;

uint8_t   S3M_flag_stereo = 0, S3M_flag_amigalimits = 0;
uint8_t   S3M_flag_st3volslides = 0, S3M_flag_note = 0;
uint8_t   S3M_tempo = 128, S3M_speed = 6, S3M_GlobalVolume = 64;

uint8_t   S3M_ChannelActiv[S3M_MAXCHANNELS];
uint8_t   S3M_SampleNr[S3M_MAXCHANNELS];
uintptr_t S3M_SampleAddress[S3M_MAXCHANNELS];
uint32_t  S3M_SampleLength[S3M_MAXCHANNELS];
uint32_t  S3M_SamplePosition[S3M_MAXCHANNELS];
uint32_t  S3M_SampleFraction[S3M_MAXCHANNELS];
uint32_t  S3M_SampleLoopBegin[S3M_MAXCHANNELS];
uint32_t  S3M_SampleLoopEnd[S3M_MAXCHANNELS];
uint32_t  S3M_SampleC4SPD[S3M_MAXCHANNELS];
uint8_t   S3M_SampleFlags[S3M_MAXCHANNELS];

uint8_t   S3M_Volume[S3M_MAXCHANNELS];
uint8_t   S3M_Panning[S3M_MAXCHANNELS];
uint8_t   S3M_Note[S3M_MAXCHANNELS];
uint8_t   S3M_Octave[S3M_MAXCHANNELS];
uint8_t   S3M_Effect[S3M_MAXCHANNELS];
uint8_t   S3M_EffectInfo[S3M_MAXCHANNELS];
uint8_t   S3M_LastEffect[S3M_MAXCHANNELS];
uint8_t   S3M_LastEffectInfo[S3M_MAXCHANNELS];

uint32_t  S3M_Periode[S3M_MAXCHANNELS];
int32_t   S3M_PeriodeAdjust[S3M_MAXCHANNELS];
uint32_t  S3M_Frequence[S3M_MAXCHANNELS];

/* ---- internal workspace pointers ---- */
static uint8_t       *s_buf      = NULL;
static uint8_t      **s_ins      = NULL;   /* InstrumentPointer[] */
static uint8_t      **s_smp      = NULL;   /* SamplePointer[]     */
static uint8_t      **s_pat      = NULL;   /* PatternPointer[]    */
static const uint8_t *s_order    = NULL;
static const uint8_t *s_chansets = NULL;
static uint8_t        s_dat_ready = 0;     /* 0=unloaded, 1=playing, 2=done */

/* ---- S3M on-disk header layout (64 bytes) ---- */
#pragma pack(push,1)
typedef struct {
    char     song_name[28];
    uint8_t  value_1ah;
    uint8_t  type;
    uint16_t dummy;
    uint16_t ord_num;
    uint16_t ins_num;
    uint16_t pat_num;
    uint16_t flags;
    uint16_t tracker_version;
    uint16_t file_format_info;
    char     scrm[4];
    uint8_t  global_volume;
    uint8_t  init_speed;
    uint8_t  init_tempo;
    uint8_t  master_volume;
    uint8_t  ultra_click;
    uint8_t  default_panning;
    uint8_t  dummy2[8];
    uint16_t special_pointer;
} s3m_header_t;
#pragma pack(pop)

/* ---- pattern decode state ---- */
static uint32_t s_last_pattern = 0xFFFFFFFFu;
static uint32_t s_last_row     = 0xFFFFFFFFu;
static uint8_t *s_last_row_ptr = NULL;

/* ---- conversion helpers (replaces convert.cpp) ---- */
static void conv_u8m_to_s8m(uint8_t *data, uint32_t length)
{
    uint32_t i;
    for (i = 0; i < length; i++) data[i] ^= 0x80u;
}

static void conv_u16m_to_s16m(uint8_t *data, uint32_t length)
{
    uint16_t *s = (uint16_t *)data;
    uint32_t i;
    for (i = 0; i < length; i++) s[i] ^= 0x8000u;
}

/* ---- workspace sizing ---- */
size_t s3m_workspace_bytes(const uint8_t *data, size_t size)
{
    const s3m_header_t *hdr;
    size_t ptr_align, ws;
    if (size < 64) return 0;
    hdr = (const s3m_header_t *)data;
    ptr_align = sizeof(uint8_t *) - 1u;
    ws  = size;
    ws  = (ws + ptr_align) & ~ptr_align;
    ws += (size_t)hdr->ins_num * sizeof(uint8_t *) * 2u;
    ws += (size_t)hdr->pat_num * sizeof(uint8_t *);
    ws += 65544u;   /* MasterVolumeTable: 16384 entries * 4 bytes * 2 sides + guard */
    ws += 128u;     /* alignment overhead */
    return ws;
}

/* ---- loader ---- */
int s3m_load(const uint8_t *data, size_t size,
             uint8_t *ws, size_t ws_size)
{
    const s3m_header_t *hdr = (const s3m_header_t *)data;
    uint8_t *p;
    size_t ptr_align, module_aligned;
    uint32_t channel, i;
    const uint16_t *tp;

    s_dat_ready = 0;

    if (size < 64)                                 return -1;
    if (memcmp(hdr->scrm, "SCRM", 4) != 0)        return -1;
    if (hdr->value_1ah != 0x1a)                    return -1;
    if (hdr->type      != 16)                      return -1;
    if (hdr->ord_num < 2)                          return -1;
    if (hdr->ins_num  == 0)                        return -1;
    if (hdr->pat_num  == 0)                        return -1;
    if (ws_size < s3m_workspace_bytes(data, size)) return -2;

    /* carve workspace */
    ptr_align      = sizeof(uint8_t *) - 1u;
    module_aligned = (size + ptr_align) & ~ptr_align;

    s_buf = ws;
    memcpy(s_buf, data, size);

    p     = ws + module_aligned;
    s_ins = (uint8_t **)p;
    p    += hdr->ins_num * sizeof(uint8_t *);
    s_smp = (uint8_t **)p;
    p    += hdr->ins_num * sizeof(uint8_t *);
    s_pat = (uint8_t **)p;
    p    += hdr->pat_num * sizeof(uint8_t *);
    /* align to 8 bytes for int32_t table */
    p = (uint8_t *)(((uintptr_t)p + 7u) & ~(uintptr_t)7u);
    /* centre of MasterVolumeTable at index 0 */
    g_master_vol_table = (int32_t *)p + 8192;

    s_chansets = s_buf + 0x40;
    s_order    = s_buf + 0x60;

    /* build instrument and sample pointer tables */
    /* Use byte-safe reads — ord_num may be odd, causing misaligned uint16_t access */
    tp = (const uint16_t *)(s_buf + 0x60 + hdr->ord_num);
    for (i = 0; i < hdr->ins_num; i++) {
        uint32_t para, pp;
        const uint8_t *tp8 = (const uint8_t *)tp + i * 2u;
        pp = (uint32_t)tp8[0] | ((uint32_t)tp8[1] << 8);
        s_ins[i] = s_buf + (pp << 4);
        para = (uint32_t)(s_ins[i][0x0e]) | ((uint32_t)(s_ins[i][0x0f]) << 8)
             | ((uint32_t)(*(const uint8_t *)(s_ins[i] + 0x0d)) << 16);
        s_smp[i] = s_buf + (para << 4);
        *(uint8_t *)(s_ins[i] + 0x0d) = 0;  /* terminate sample name */
    }

    {
        const uint8_t *tp8 = (const uint8_t *)tp + hdr->ins_num * 2u;
        for (i = 0; i < hdr->pat_num; i++) {
            uint32_t pp = (uint32_t)tp8[i*2] | ((uint32_t)tp8[i*2+1] << 8);
            s_pat[i] = (pp != 0) ? (s_buf + (pp << 4)) : NULL;
        }
    }

    /* convert unsigned samples to signed in-place */
    if (hdr->file_format_info == 2) {
        for (i = 0; i < hdr->ins_num; i++) {
            uint8_t  flags;
            uint32_t len;
            if (*s_ins[i] != 1) continue;
            flags = *(s_ins[i] + 0x1f);
            len   = *(uint32_t *)(s_ins[i] + 0x10);
            if ((flags & 4) == 0) {
                if ((flags & 2) == 0) conv_u8m_to_s8m(s_smp[i], len);
            } else {
                if ((flags & 2) == 0) conv_u16m_to_s16m(s_smp[i], len);
            }
        }
    }

    /* init global song state */
    S3M_OrderNum      = hdr->ord_num;
    S3M_InstrumentNum = hdr->ins_num;
    S3M_PatternNum    = hdr->pat_num;

    S3M_initvariables();
    S3M_initEffects();

    S3M_GlobalVolume      = hdr->global_volume;
    g_MasterVolume        = (uint32_t)(hdr->master_volume & 127u) * 256u;
    S3M_flag_stereo       = hdr->master_volume >> 7;
    S3M_flag_amigalimits  = (hdr->flags & 16u) >> 4;
    S3M_flag_st3volslides = (hdr->flags & 64u) >> 6;
    if (hdr->tracker_version == 0x1300) S3M_flag_st3volslides = 1;

    S3M_tempo = hdr->init_tempo;
    S3M_speed = hdr->init_speed;

    S3M_channels = 0;
    for (channel = 0; channel < S3M_MAXCHANNELS; channel++)
        if ((s_chansets[channel] & 127u) < 16u) S3M_channels++;

    /* default panning */
    if (hdr->default_panning == 0xFC) {
        const uint8_t *defpan = s_order + hdr->ord_num
                              + hdr->ins_num * 2u + hdr->pat_num * 2u;
        for (channel = 0; channel < S3M_MAXCHANNELS; channel++) {
            if (S3M_flag_stereo)
                S3M_Panning[channel] = 0x03u
                    + ((s_chansets[channel] & 15u) >> 3) * 0x09u;
            if (defpan[channel] & 32u)
                S3M_Panning[channel] = defpan[channel] & 15u;
        }
    }

    /* timing */
    S3M_TickLength = s3m_calc_speed(S3M_tempo, g_MixSpeed);
    S3M_row = 0; S3M_jump = 0; S3M_tick = 0;
    S3M_tick_rest = S3M_TickLength;
    S3M_Order   = 0;
    S3M_Pattern = s_order[0];

    while (S3M_Pattern > 199) {
        S3M_Order++;
        S3M_Pattern = s_order[S3M_Order];
        if (S3M_Pattern == 255) return -1;
    }

    S3M_unpack_row(S3M_Pattern, S3M_row);
    S3M_read_row();

    mixer_calc_master_vol32(g_MasterVolume, g_master_vol_table);

    s_dat_ready = 1;
    return 0;
}

void s3m_close(void)
{
    s_dat_ready = 0;
    s_buf = NULL; s_ins = NULL; s_smp = NULL; s_pat = NULL;
    s_order = NULL; s_chansets = NULL;
    g_master_vol_table = NULL;
    s_last_pattern = 0xFFFFFFFFu;
    s_last_row     = 0xFFFFFFFFu;
    s_last_row_ptr = NULL;
}

uint8_t s3m_is_done(void) { return (s_dat_ready == 2); }

/* ---- pattern decoding ---- */

static void clear_row(void)
{
    uint32_t counter;
    for (counter = 0; counter < 160; counter += 20) {
        *(uint32_t *)(S3M_RowBuffer + counter + 0)  = 0xffff00ffu;
        *(uint32_t *)(S3M_RowBuffer + counter + 4)  = 0xff00ff00u;
        *(uint32_t *)(S3M_RowBuffer + counter + 8)  = 0x00ff00ffu;
        *(uint32_t *)(S3M_RowBuffer + counter + 12) = 0xff00ffffu;
        *(uint32_t *)(S3M_RowBuffer + counter + 16) = 0x00ffff00u;
    }
}

static void unpack_first_row(uint8_t *pat_addr)
{
    uint8_t *ptr;
    uint8_t  val;
    uint32_t ch;

    S3M_nextrow = 0;
    clear_row();

    if (pat_addr == NULL) {
        s_last_row_ptr = NULL;
        return;
    }

    ptr = pat_addr + 2;

    while (*ptr != 0) {
        val = *ptr++;
        if (val != 0) {
            ch = val & 31u;
            if (val & 32u) {
                S3M_RowBuffer[ch * 5 + 0] = *ptr++;
                S3M_RowBuffer[ch * 5 + 1] = *ptr++;
            }
            if (val & 64u)  S3M_RowBuffer[ch * 5 + 2] = *ptr++;
            if (val & 128u) {
                S3M_RowBuffer[ch * 5 + 3] = *ptr++;
                S3M_RowBuffer[ch * 5 + 4] = *ptr++;
            }
        }
    }
    s_last_row_ptr = ptr + 1;
}

static void unpack_next_row(void)
{
    uint8_t *ptr;
    uint8_t  val;
    uint32_t ch;

    clear_row();
    if (s_last_row_ptr == NULL) return;

    ptr = s_last_row_ptr;

    while (*ptr != 0) {
        val = *ptr++;
        if (val != 0) {
            ch = val & 31u;
            if (val & 32u) {
                S3M_RowBuffer[ch * 5 + 0] = *ptr++;
                S3M_RowBuffer[ch * 5 + 1] = *ptr++;
            }
            if (val & 64u)  S3M_RowBuffer[ch * 5 + 2] = *ptr++;
            if (val & 128u) {
                S3M_RowBuffer[ch * 5 + 3] = *ptr++;
                S3M_RowBuffer[ch * 5 + 4] = *ptr++;
            }
        }
    }
    s_last_row_ptr = ptr + 1;
}

void S3M_unpack_row(uint32_t pat_nr, uint32_t row_nr)
{
    uint32_t counter;

    if (pat_nr >= S3M_PatternNum) {
        clear_row();
        s_last_pattern = pat_nr;
        s_last_row     = row_nr;
        return;
    }

    if (pat_nr == s_last_pattern) {
        if (row_nr == s_last_row) return;
        if (row_nr == s_last_row + 1) {
            unpack_next_row();
        } else if (row_nr < s_last_row) {
            unpack_first_row(s_pat[pat_nr]);
            for (counter = 0; counter < row_nr; counter++) unpack_next_row();
        } else {
            for (counter = 0; counter < (row_nr - s_last_row); counter++)
                unpack_next_row();
        }
    } else {
        unpack_first_row(s_pat[pat_nr]);
        for (counter = 0; counter < row_nr; counter++) unpack_next_row();
    }

    s_last_pattern = pat_nr;
    s_last_row     = row_nr;
}

/* ---- period / frequency calculations ---- */

uint32_t Calc_st3periode(uint32_t note, uint32_t octave, uint32_t c4spd)
{
    uint32_t noteperiode;
    uint32_t dum;
    if (note > 11 || c4spd == 0) return 0x1000u;  /* safe fallback */
    noteperiode = S3M_NotePeriodes[note] << 4;
    dum = (uint32_t)(8363u * 16u * (noteperiode >> octave));
    return dum / c4spd;
}

static uint32_t calc_st3frequence(uint32_t channel)
{
    uint32_t dum = S3M_Periode[channel];
    if (dum < 1024)      dum = 1024;
    if (dum > 0x00080000u) dum = 0x00080000u;
    return (14317456ul << 6) / (dum + (uint32_t)S3M_PeriodeAdjust[channel]);
}

/* ---- note / sample setup ---- */

void S3M_GetNewSample(uint32_t channel)
{
    uint32_t sample_nr;
    uint8_t  sample_bits, sample_stereo;

    if (S3M_SampleNr[channel] == 0) return;

    sample_nr = S3M_SampleNr[channel] - 1u;

    if (sample_nr < S3M_InstrumentNum && *s_ins[sample_nr] == 1) {
        uint8_t flags = *(s_ins[sample_nr] + 0x1f);
        sample_bits   = (flags & 4u) >> 2;
        sample_stereo = (flags & 2u) >> 1;
        S3M_SampleAddress[channel]   = (uintptr_t)s_smp[sample_nr];
        S3M_SampleFlags[channel]     = flags;
        S3M_SampleLength[channel]    = *(uint32_t *)(s_ins[sample_nr] + 0x10)
                                       >> (sample_bits + sample_stereo);
        S3M_SampleLoopBegin[channel] = *(uint32_t *)(s_ins[sample_nr] + 0x14)
                                       >> (sample_bits + sample_stereo);
        S3M_SampleLoopEnd[channel]   = *(uint32_t *)(s_ins[sample_nr] + 0x18)
                                       >> (sample_bits + sample_stereo);
        S3M_SampleC4SPD[channel]     = *(uint32_t *)(s_ins[sample_nr] + 0x20);
        S3M_TremorVolume[channel]    =
        S3M_Volume[channel]          =
            (uint8_t)((S3M_GlobalVolume * *(s_ins[sample_nr] + 0x1c) + 32u) / 64u);
        return;
    }
    S3M_SampleNr[channel] = 0;
}

void S3M_GetNewNote(uint32_t channel)
{
    if (S3M_SampleNr[channel] != 0) {
        if (S3M_Note[channel] > 11) {
            S3M_Periode[channel] = 0;
            S3M_ChannelActiv[channel] = 0;
        } else {
            S3M_Periode[channel] = S3M_GlissPeriode[channel] =
                Calc_st3periode(S3M_Note[channel], S3M_Octave[channel],
                                S3M_SampleC4SPD[channel]);
            S3M_SamplePosition[channel] = 0;
            S3M_SampleFraction[channel] = 0;
            S3M_ChannelActiv[channel]   = 1;
        }
    } else {
        S3M_ChannelActiv[channel] = 0;
    }
}

/* ---- effect dispatch (called each row) ---- */

void S3M_GetNewEffect(void)
{
    uint32_t channel;
    uint8_t  effect, info;

    for (channel = 0; channel < S3M_channels; channel++) {
        effect = S3M_Effect[channel];
        info   = S3M_EffectInfo[channel];

        switch (effect) {
            case 255: S3M_PeriodeAdjust[channel] = 0; break;
            case  1: case  2: case  3:
                S3M_VibratoPosition[channel] = 0; break;
            case  4:
                S3M_PeriodeAdjust[channel] = 0; break;
            case  5: case  6:
                S3M_Periode[channel] += (uint32_t)S3M_PeriodeAdjust[channel];
                S3M_PeriodeAdjust[channel] = 0;
                S3M_VibratoPosition[channel] = 0; break;
            case  7: S3M_VibratoPosition[channel] = 0; break;
            case  8: break;
            case  9: case 10:
                S3M_VibratoPosition[channel] = 0; break;
            case 11: break;
            case 12: case 13: case 14: case 15: case 16: case 17:
                S3M_VibratoPosition[channel] = 0; break;
            case 18: break;
            case 19: case 20:
                S3M_VibratoPosition[channel] = 0; break;
            case 21: break;
            case 22: case 23: case 24: case 25: case 26:
                S3M_VibratoPosition[channel] = 0; break;
            default: break;
        }

        if (info == 0) {
            switch (effect) {
                case 255: case  1: case  2: case  3: break;
                case  4: case  5: case  6:
                    S3M_EffectInfo[channel] = S3M_LastEffectInfo[channel]; break;
                case  7: case  8: break;
                case  9: case 10: case 11: case 12: case 13:
                case 14: case 15: case 16: case 17:
                    S3M_EffectInfo[channel] = S3M_LastEffectInfo[channel]; break;
                default: break;
            }
        }
    }

    S3M_GlobalEffect();
    S3M_beforeEffect();
}

void S3M_read_row(void)
{
    uint32_t channel;
    uint32_t value;

    for (channel = 0; channel < S3M_channels; channel++) {
        value = S3M_RowBuffer[channel * 5 + 4];
        if (S3M_EffectInfo[channel] != 0)
            S3M_LastEffectInfo[channel] = S3M_EffectInfo[channel];
        S3M_EffectInfo[channel] = (uint8_t)value;

        value = S3M_RowBuffer[channel * 5 + 3];
        if (S3M_Effect[channel] != 255)
            S3M_LastEffect[channel] = S3M_Effect[channel];
        S3M_Effect[channel] = (uint8_t)value;
    }

    S3M_GetNewEffect();

    if (S3M_flag_note == 0) {
        for (channel = 0; channel < S3M_channels; channel++) {
            value = S3M_RowBuffer[channel * 5 + 1];
            if (value != 0) {
                S3M_SampleNr[channel] = (uint8_t)value;
                S3M_GetNewSample(channel);
            }

            value = S3M_RowBuffer[channel * 5 + 2];
            if (value != 255) {
                S3M_Volume[channel] =
                    (uint8_t)((S3M_GlobalVolume * value + 32u) / 64u);
                S3M_TremorVolume[channel] = (uint16_t)value;
            }

            value = S3M_RowBuffer[channel * 5 + 0];
            if (value != 255) {
                if (value == 254) {
                    S3M_ChannelActiv[channel] = 0;
                } else {
                    S3M_Note[channel]   = (uint8_t)(value & 15u);
                    S3M_Octave[channel] = (uint8_t)(value >> 4);
                    S3M_GetNewNote(channel);
                    S3M_VibratoPosition[channel] = 0;
                    S3M_PeriodeAdjust[channel]   = 0;
                }
            }
        }
    }

    S3M_RowEffect();
}

void S3M_goRowOrder(void)
{
    if (s_dat_ready != 1) return;

    if (S3M_jump == 0) {
        if (S3M_flag_note == 0) S3M_row++;

        if (S3M_row == 64) {
            S3M_Order++;
            S3M_row = 0;
            S3M_Pattern = s_order[S3M_Order];

            while (S3M_Pattern == 254) {
                S3M_Order++;
                S3M_Pattern = s_order[S3M_Order];
            }

            if (S3M_Pattern == 255) {
                S3M_Order   = 0;
                S3M_Pattern = s_order[0];
                s_dat_ready = 2;
            }
        }
    } else {
        S3M_jump = 0;
        if (S3M_nextrow <= 63 && S3M_nextorder <= S3M_OrderNum) {
            S3M_row     = S3M_nextrow;
            S3M_Order   = S3M_nextorder;
            S3M_Pattern = s_order[S3M_Order];

            while (S3M_Pattern == 254) {
                S3M_Order++;
                S3M_Pattern = s_order[S3M_Order];
            }

            if (S3M_Pattern == 255) {
                S3M_Order   = 0;
                S3M_Pattern = s_order[0];
                s_dat_ready = 2;
            }
        }
        S3M_nextrow   = 0;
        S3M_nextorder = 0;
    }
}

void S3M_initvariables(void)
{
    uint32_t channel;

    s_last_pattern = 0xFFFFFFFFu;
    s_last_row     = 0xFFFFFFFFu;

    S3M_TickLength  = 0;
    S3M_row = 0; S3M_jump = 0; S3M_nextrow = 0; S3M_nextorder = 0;
    S3M_tick = 0; S3M_Order = 0; S3M_Pattern = 0;
    S3M_buffer_rest = 0; S3M_tick_rest = 0;

    S3M_flag_stereo = 0; S3M_flag_amigalimits = 0;
    S3M_flag_st3volslides = 0; S3M_flag_note = 0;

    S3M_tempo = 128; S3M_speed = 6; S3M_GlobalVolume = 64;

    for (channel = 0; channel < S3M_MAXCHANNELS; channel++) {
        S3M_ChannelActiv[channel]   = 0;
        S3M_SampleNr[channel]       = 0;
        S3M_SampleAddress[channel]  = 0;
        S3M_SampleLength[channel]   = 0;
        S3M_SamplePosition[channel] = 0;
        S3M_SampleFraction[channel] = 0;
        S3M_SampleLoopBegin[channel]= 0;
        S3M_SampleLoopEnd[channel]  = 0;
        S3M_SampleC4SPD[channel]    = 0;
        S3M_SampleFlags[channel]    = 0;
        S3M_Volume[channel]         = 0;
        S3M_Panning[channel]        = 7;
        S3M_Note[channel]           = 0xff;
        S3M_Octave[channel]         = 0;
        S3M_Effect[channel]         = 0xff;
        S3M_EffectInfo[channel]     = 0;
        S3M_LastEffect[channel]     = 0xff;
        S3M_LastEffectInfo[channel] = 0;
        S3M_Periode[channel]        = 0;
        S3M_PeriodeAdjust[channel]  = 0;
        S3M_Frequence[channel]      = 0;
    }
}

/* ---- mixer bridge ---- */

static void s3m_to_mixer(void)
{
    uint32_t channel;
    g_ChannelLast = S3M_channels;
    for (channel = 0; channel < S3M_channels; channel++) {
        if (S3M_Periode[channel] < 60) S3M_ChannelActiv[channel] = 0;
        g_ChannelActiv[channel]  = S3M_ChannelActiv[channel];
        g_ChannelVolume[channel] = (uint32_t)S3M_Volume[channel] << 2;
        g_ChannelPanning[channel]= S3M_PanningTable[S3M_Panning[channel]];
        if (S3M_ChannelActiv[channel]) {
            g_ChannelSamplePosition[channel]  = S3M_SamplePosition[channel];
            g_ChannelSampleFraction[channel]  = S3M_SampleFraction[channel];
            S3M_Frequence[channel]            = calc_st3frequence(channel);
            g_ChannelSampleFrequence[channel] = (S3M_Frequence[channel] + 2u) >> 2;
            if (S3M_SampleNr[channel] != 0 &&
                g_ChannelSampleNr[channel] != S3M_SampleNr[channel]) {
                g_ChannelSampleNr[channel]      = S3M_SampleNr[channel];
                g_ChannelSampleBits[channel]    =
                    ((S3M_SampleFlags[channel] & 4u) << 1) + 8u;
                g_ChannelSampleMode[channel]    =
                    (S3M_SampleFlags[channel] & 2u) >> 1;
                g_ChannelSampleAddress[channel] =
                    S3M_SampleAddress[channel];  /* already uintptr_t */
                g_ChannelSampleLength[channel]  = S3M_SampleLength[channel];
                g_ChannelLoopMode[channel]      = S3M_SampleFlags[channel] & 1u;
                g_ChannelLoopBegin[channel]     = S3M_SampleLoopBegin[channel];
                g_ChannelLoopEnd[channel]       = S3M_SampleLoopEnd[channel];
            }
        }
    }
}

static void mixer_to_s3m(void)
{
    uint32_t channel;
    for (channel = 0; channel < S3M_channels; channel++) {
        if (S3M_ChannelActiv[channel]) {
            S3M_SamplePosition[channel] = g_ChannelSamplePosition[channel];
            S3M_SampleFraction[channel] = g_ChannelSampleFraction[channel];
            S3M_ChannelActiv[channel]   = (uint8_t)g_ChannelActiv[channel];
        }
    }
}

void s3m_render_block(uint32_t block_frames)
{
    uint32_t toplay;
    S3M_buffer_rest = block_frames;
    g_MixerAddress  = 0;
    mixer_clear();

    while (S3M_buffer_rest > 0) {
        if (S3M_tick_rest <= S3M_buffer_rest) {
            toplay = S3M_tick_rest;
            s3m_to_mixer();
            mixer_do_pre_mixing(toplay);
            mixer_to_s3m();
            g_MixerAddress += toplay;
            S3M_tick++;
            if (S3M_tick == S3M_speed) {
                S3M_tick = 0;
                S3M_goRowOrder();
                S3M_unpack_row(S3M_Pattern, S3M_row);
                if (s_dat_ready == 1) S3M_read_row();
            } else {
                S3M_TickEffect();
            }
            S3M_buffer_rest -= toplay;
            S3M_tick_rest    = S3M_TickLength;
        } else {
            toplay = S3M_buffer_rest;
            s3m_to_mixer();
            mixer_do_pre_mixing(toplay);
            mixer_to_s3m();
            g_MixerAddress += toplay;
            S3M_tick_rest  -= toplay;
            S3M_buffer_rest = 0;
        }
    }
}
