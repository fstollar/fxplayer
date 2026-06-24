#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "format/m669.h"
#include "effect/efc_669.h"
#include "mixer/mixer_scalar.h"
#include "util/calc.h"

/*
 * Period table [16 finetunes][72 notes].
 * Seed rows 12..47 (octaves 1-3); load time extends to full 6-octave range.
 * Layout: [0..11]=oct0, [12..23]=oct1(seed), [24..35]=oct2(seed),
 *         [36..47]=oct3(seed), [48..59]=oct4, [60..71]=oct5.
 */
uint32_t M669_Periods[16][72] = {
/* Tuning 0 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +1 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    850,802,757,715,674,637,601,567,535,505,477,450,
    425,401,379,357,337,318,300,284,268,253,239,225,
    213,201,189,179,169,159,150,142,134,126,119,113,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +2 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    844,796,752,709,670,632,597,563,532,502,474,447,
    422,398,376,355,335,316,298,282,266,251,237,224,
    211,199,188,177,167,158,149,141,133,125,118,112,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +3 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    838,791,746,704,665,628,592,559,528,498,470,444,
    419,395,373,352,332,314,296,280,264,249,235,222,
    209,198,187,176,166,157,148,140,132,125,118,111,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +4 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    832,785,741,699,660,623,588,555,524,495,467,441,
    416,392,370,350,330,312,294,278,262,247,233,220,
    208,196,185,175,165,156,147,139,131,124,117,110,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +5 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    826,779,736,694,655,619,584,551,520,491,463,437,
    413,390,368,347,328,309,292,276,260,245,232,219,
    206,195,184,174,164,155,146,138,130,123,116,109,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +6 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    820,774,730,689,651,614,580,547,516,487,460,434,
    410,387,365,345,325,307,290,274,258,244,230,217,
    205,193,183,172,163,154,145,137,129,122,115,109,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning +7 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    814,768,725,684,646,610,575,543,513,484,457,431,
    407,384,363,342,323,305,288,272,256,242,228,216,
    204,192,181,171,161,152,144,136,128,121,114,108,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -8 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    907,856,808,762,720,678,640,604,570,538,508,480,
    453,428,404,381,360,339,320,302,285,269,254,240,
    226,214,202,190,180,170,160,151,143,135,127,120,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -7 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    900,850,802,757,715,675,636,601,567,535,505,477,
    450,425,401,379,357,337,318,300,284,268,253,238,
    225,212,200,189,179,169,159,150,142,134,126,119,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -6 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    894,844,796,752,709,670,632,597,563,532,502,474,
    447,422,398,376,355,335,316,298,282,266,251,237,
    223,211,199,188,177,167,158,149,141,133,125,118,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -5 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    887,838,791,746,704,665,628,592,559,528,498,470,
    444,419,395,373,352,332,314,296,280,264,249,235,
    222,209,198,187,176,166,157,148,140,132,125,118,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -4 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    881,832,785,741,699,660,623,588,555,524,494,467,
    441,416,392,370,350,330,312,294,278,262,247,233,
    220,208,196,185,175,165,156,147,139,131,123,117,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -3 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    875,826,779,736,694,655,619,584,551,520,491,463,
    437,413,390,368,347,328,309,292,276,260,245,232,
    219,206,195,184,174,164,155,146,138,130,123,116,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -2 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    868,820,774,730,689,651,614,580,547,516,487,460,
    434,410,387,365,345,325,307,290,274,258,244,230,
    217,205,193,183,172,163,154,145,137,129,122,115,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 },
/* Tuning -1 */
  { 0,0,0,0,0,0,0,0,0,0,0,0,
    862,814,768,725,684,646,610,575,543,513,484,457,
    431,407,384,363,342,323,305,288,272,256,242,228,
    216,203,192,181,171,161,152,144,136,128,121,114,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0 }
};

/* ---- song state ---- */

uint32_t M669_samples  = 0;
uint32_t M669_patterns = 0;
uint8_t  M669_OrderNum = 0;

uint8_t  M669_RowBuffer[M669_CHANNELS * 5];

uint32_t M669_LastPattern = 0xFFFFu;
uint32_t M669_LastRow     = 0xFFFFu;
uint32_t M669_TickLength  = 0;

uint32_t M669_row = 0, M669_jump = 0, M669_nextrow = 0, M669_nextorder = 0;
uint32_t M669_tick = 0, M669_Order = 0, M669_Pattern = 0;
uint32_t M669_buffer_rest = 0, M669_tick_rest = 0;

uint8_t  M669_tempo = 78;
uint8_t  M669_speed = 6;

uint8_t  M669_ChannelActive[M669_CHANNELS];
uint8_t  M669_SampleNr[M669_CHANNELS];
uintptr_t M669_SampleAddress[M669_CHANNELS];
uint32_t M669_SampleLength[M669_CHANNELS];
uint32_t M669_SamplePosition[M669_CHANNELS];
uint32_t M669_SampleFraction[M669_CHANNELS];
uint8_t  M669_SampleLoop[M669_CHANNELS];
uint32_t M669_SampleLoopBegin[M669_CHANNELS];
uint32_t M669_SampleLoopEnd[M669_CHANNELS];
uint32_t M669_SampleFinetune[M669_CHANNELS];

uint8_t  M669_Volume[M669_CHANNELS];
uint8_t  M669_Panning[M669_CHANNELS];
uint8_t  M669_Note[M669_CHANNELS];
uint8_t  M669_Effect[M669_CHANNELS];
uint8_t  M669_EffectInfo[M669_CHANNELS];
uint8_t  M669_LastEffect[M669_CHANNELS];
uint8_t  M669_LastEffectInfo[M669_CHANNELS];

uint32_t M669_Period[M669_CHANNELS];
int32_t  M669_PeriodAdjust[M669_CHANNELS];
uint32_t M669_Frequency[M669_CHANNELS];

uint8_t *M669_Orderlist = NULL;
uint8_t *M669_Speedlist = NULL;
uint8_t *M669_Breaklist = NULL;

/* ---- internal workspace pointers ---- */
static uint8_t  *s_buf    = NULL;
static uint8_t **s_ins    = NULL;
static uint8_t **s_smp    = NULL;
static uint8_t **s_pat    = NULL;
static uint8_t   s_dat_ready  = 0;
static uint32_t  s_song_loops = 0;

/* ---- workspace sizing ---- */

/*
 * 669 header: 0x1f1 bytes.
 * Instrument blocks: samples * 0x19 bytes.
 * Pattern data: patterns * 0x600 bytes.
 * Sample audio follows.
 * We use max (64 samples, 128 patterns) for sizing.
 */
size_t m669_workspace_bytes(const uint8_t *data, size_t size)
{
    size_t ptr_align, ws;
    (void)data;
    if (size < 0x1f1u) return 0;
    ptr_align = sizeof(uint8_t *) - 1u;
    ws  = size;
    ws  = (ws + ptr_align) & ~ptr_align;
    ws += (size_t)M669_MAXSAMPLES  * sizeof(uint8_t *) * 2u;  /* ins + smp */
    ws += (size_t)M669_MAXPATTERNS * sizeof(uint8_t *);        /* pat */
    ws += 65544u;   /* MasterVolumeTable */
    ws += 128u;     /* alignment overhead */
    return ws;
}

/* ---- loader ---- */

int m669_load(const uint8_t *data, size_t size,
              uint8_t *ws, size_t ws_size)
{
    uint8_t *p;
    size_t ptr_align, module_aligned;
    uint32_t i, t;

    s_dat_ready  = 0;
    s_song_loops = 0;

    if (size < 0x1f1u)  return -1;
    if (ws_size < m669_workspace_bytes(data, size)) return -2;

    /* validate magic "if" */
    if (data[0] != 0x69u || data[1] != 0x66u) return -1;

    /* carve workspace */
    ptr_align      = sizeof(uint8_t *) - 1u;
    module_aligned = (size + ptr_align) & ~ptr_align;

    s_buf = ws;
    memcpy(s_buf, data, size);

    p     = ws + module_aligned;
    s_ins = (uint8_t **)p;
    p    += (size_t)M669_MAXSAMPLES * sizeof(uint8_t *);
    s_smp = (uint8_t **)p;
    p    += (size_t)M669_MAXSAMPLES * sizeof(uint8_t *);
    s_pat = (uint8_t **)p;
    p    += (size_t)M669_MAXPATTERNS * sizeof(uint8_t *);
    p     = (uint8_t *)(((uintptr_t)p + 7u) & ~(uintptr_t)7u);
    g_master_vol_table = (int32_t *)p + 8192;

    /* extend period table: octave 0 and 4-5 from seeded octaves 1-3 */
    for (t = 0; t < 16u; t++) {
        for (i = 0; i < 12u; i++) {
            M669_Periods[t][i]    = M669_Periods[t][i + 12] * 2u;
            M669_Periods[t][i+48] = (M669_Periods[t][i + 36] + 1u) / 2u;
            M669_Periods[t][i+60] = (M669_Periods[t][i + 36] + 2u) / 4u;
        }
    }

    /* null-terminate song title */
    s_buf[109] = 0;

    /* parse header fields */
    M669_samples  = s_buf[0x6eu];
    M669_patterns = s_buf[0x6fu];
    M669_OrderNum = s_buf[0x70u];

    if (M669_samples  > M669_MAXSAMPLES)  return -1;
    if (M669_patterns > M669_MAXPATTERNS) return -1;

    M669_Orderlist = s_buf + 0x71u;
    M669_Speedlist = s_buf + 0xf1u;
    M669_Breaklist = s_buf + 0x171u;

    /* build instrument and sample pointers */
    {
        uint8_t *ins_base = s_buf + 0x1f1u;
        uint32_t smp_offset = (uint32_t)M669_samples * 0x19u +
                              (uint32_t)M669_patterns * 0x600u;

        for (i = 0; i < M669_samples; i++) {
            uint32_t sample_length;
            s_ins[i] = ins_base + i * 0x19u;
            s_smp[i] = ins_base + smp_offset;
            sample_length = *(uint32_t *)(s_ins[i] + 13);
            /* convert samples from unsigned to signed in-place */
            {
                uint8_t *d = s_smp[i];
                uint32_t j;
                for (j = 0; j < sample_length; j++) d[j] ^= 0x80u;
            }
            smp_offset += sample_length;
            if ((size_t)(ins_base + smp_offset - s_buf) > size) return -1;
        }
    }

    /* build pattern pointers */
    {
        uint8_t *pat_base = s_buf + 0x1f1u + (uint32_t)M669_samples * 0x19u;
        for (i = 0; i < M669_patterns; i++) {
            s_pat[i] = pat_base + i * 0x600u;
        }
    }

    /* init state */
    M669_initvariables();
    M669_initEffects();

    g_MasterVolume = 12288u;

    M669_tempo = 78;
    M669_speed = M669_Speedlist[0];

    /* alternating L/R panning */
    for (i = 0; i < M669_CHANNELS; i++) {
        M669_Panning[i] = (i & 1u) ? 200u : 56u;
    }

    M669_TickLength = s3m_calc_speed(M669_tempo, g_MixSpeed);
    M669_tick_rest  = M669_TickLength;
    M669_row        = 0;
    M669_jump       = 0;
    M669_nextrow    = 0;
    M669_nextorder  = 0;
    M669_tick       = 0;
    M669_Order      = 0;
    M669_Pattern    = M669_Orderlist[0];

    mixer_calc_master_vol32(g_MasterVolume, g_master_vol_table);

    M669_unpack_row(M669_Pattern, M669_row);
    M669_read_row();

    s_dat_ready = 1;
    return 0;
}

void m669_close(void)
{
    s_dat_ready  = 0;
    s_song_loops = 0;
    s_buf = NULL;
    s_ins = s_smp = s_pat = NULL;
    M669_Orderlist = M669_Speedlist = M669_Breaklist = NULL;
    g_master_vol_table = NULL;
}

uint8_t     m669_is_done(void)    { return s_dat_ready == 2u; }
uint32_t    m669_song_loops(void) { return s_song_loops; }
const char *m669_song_title(void) { return s_buf ? (const char *)(s_buf + 2) : ""; }
void        m669_restart(void)    { if (s_dat_ready == 2) s_dat_ready = 1; }

/* ---- initialisation ---- */

void M669_initvariables(void)
{
    uint32_t channel_index;

    M669_LastPattern = 0xFFFFu;
    M669_LastRow     = 0xFFFFu;
    M669_TickLength  = 0;
    M669_row = M669_jump = M669_nextrow = M669_nextorder = 0;
    M669_tick = M669_Order = M669_Pattern = 0;
    M669_buffer_rest = M669_tick_rest = 0;
    M669_tempo = 78;
    M669_speed = 6;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        M669_ChannelActive[channel_index]    = 0;
        M669_SampleNr[channel_index]        = 0;
        M669_SampleAddress[channel_index]   = 0;
        M669_SampleLength[channel_index]    = 0;
        M669_SamplePosition[channel_index]  = 0;
        M669_SampleFraction[channel_index]  = 0;
        M669_SampleLoop[channel_index]      = 0;
        M669_SampleLoopBegin[channel_index] = 0;
        M669_SampleLoopEnd[channel_index]   = 0;
        M669_SampleFinetune[channel_index]  = 0;
        M669_Volume[channel_index]          = 0;
        M669_Panning[channel_index]         = 128;
        M669_Note[channel_index]            = 0xFFu;
        M669_Effect[channel_index]          = 0xFFu;
        M669_EffectInfo[channel_index]      = 0;
        M669_LastEffect[channel_index]      = 0xFFu;
        M669_LastEffectInfo[channel_index]  = 0;
        M669_Period[channel_index]         = 0;
        M669_PeriodAdjust[channel_index]   = 0;
        M669_Frequency[channel_index]       = 0;
    }
}

/* ---- pattern decode ---- */

static void m669_clear_row(void)
{
    uint32_t i;
    for (i = 0; i < M669_CHANNELS; i++) {
        M669_RowBuffer[i*5+0] = 0xFFu;
        M669_RowBuffer[i*5+1] = 0xFFu;
        M669_RowBuffer[i*5+2] = 0xFFu;
        M669_RowBuffer[i*5+3] = 0xFFu;
        M669_RowBuffer[i*5+4] = 0x00u;
    }
}

void M669_unpack_row(uint32_t pat_nr, uint32_t row_nr)
{
    uint32_t channel_index;
    const uint8_t *p;

    if (pat_nr >= M669_patterns) {
        m669_clear_row();
        M669_LastPattern = pat_nr;
        M669_LastRow = row_nr;
        return;
    }

    if (pat_nr == M669_LastPattern && row_nr == M669_LastRow) return;

    /* load per-pattern speed when pattern changes */
    if (pat_nr != M669_LastPattern) {
        M669_speed = M669_Speedlist[pat_nr];
    }

    m669_clear_row();

    p = s_pat[pat_nr] + row_nr * M669_CHANNELS * 3u;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        uint32_t b0 = p[0], b1 = p[1], b2 = p[2];
        uint32_t note, inst, note_volume, effect, effinfo;

        if (b0 != 0xFFu) {
            if (b0 != 0xFEu) {
                note = b0 >> 2;
                inst = ((b0 & 0x03u) << 4) | (b1 >> 4);
                if (inst >= M669_samples) { inst = 0xFFu; note = 0xFEu; }
                note_volume  = b1 & 0x0Fu;
            } else {
                note = 0xFEu;
                inst = 0xFFu;
                note_volume  = b1 & 0x0Fu;
            }
        } else {
            note = 0xFFu;
            inst = 0xFFu;
            note_volume  = 0xFFu;
        }

        M669_RowBuffer[channel_index*5+0] = (uint8_t)note;
        M669_RowBuffer[channel_index*5+1] = (uint8_t)inst;
        M669_RowBuffer[channel_index*5+2] = (uint8_t)note_volume;

        if (b2 != 0xFFu) {
            effect  = (b2 & 0xF0u) >> 4;
            effinfo = b2 & 0x0Fu;
        } else {
            effect  = 0xFFu;
            effinfo = 0;
        }
        M669_RowBuffer[channel_index*5+3] = (uint8_t)effect;
        M669_RowBuffer[channel_index*5+4] = (uint8_t)effinfo;

        p += 3u;
    }

    M669_LastPattern = pat_nr;
    M669_LastRow     = row_nr;
}

/* ---- period / frequency helpers ---- */

uint32_t Calc_669period(uint32_t note, uint32_t finetune)
{
    return M669_Periods[finetune][note] << 4u;
}

static uint32_t Calc_669frequency(uint32_t channel)
{
    uint32_t p = M669_Period[channel] + (uint32_t)M669_PeriodAdjust[channel];
    if (p == 0) return 0;
    return (14318181UL << 4u) / p;
}

/* ---- order / row navigation ---- */

void M669_goRowOrder(void)
{
    if (s_dat_ready != 1) return;

    if (M669_jump == 0) {
        M669_row++;

        if (M669_row >= (uint32_t)(M669_Breaklist[M669_Pattern] + 1u)) {
            M669_Order++;
            M669_row    = 0;
            M669_Pattern = M669_Orderlist[M669_Order];

            if (M669_Pattern == 0xFFu) {
                M669_Order   = 0;
                M669_Pattern = M669_Orderlist[0];
                s_song_loops++;
                s_dat_ready  = 2;
            }
        }
    } else {
        M669_jump = 0;
        if (M669_nextorder < M669_OrderNum) {
            M669_Order   = M669_nextorder;
            M669_Pattern = M669_Orderlist[M669_Order];
            M669_row     = M669_nextrow;
        }
        M669_nextorder = 0;
        M669_nextrow   = 0;
    }
}

/* ---- sample / note triggers ---- */

void M669_GetNewSample(uint32_t channel)
{
    uint32_t sn, sample_length;
    uint8_t *ins;

    if (M669_SampleNr[channel] == 0) return;
    sn = M669_SampleNr[channel] - 1u;
    if (sn >= M669_samples) { M669_SampleNr[channel] = 0; return; }

    ins = s_ins[sn];
    sample_length = *(uint32_t *)(ins + 13);
    if (sample_length == 0) { M669_SampleNr[channel] = 0; return; }

    M669_SampleAddress[channel]   = (uintptr_t)s_smp[sn];
    M669_SampleLength[channel]    = sample_length;
    M669_SampleLoopBegin[channel] = *(uint32_t *)(ins + 17);
    M669_SampleLoopEnd[channel]   = *(uint32_t *)(ins + 21);
    M669_SampleLoop[channel]      = (M669_SampleLoopEnd[channel] != 0x000FFFFFu) ? 1u : 0u;
    M669_SampleFinetune[channel]  = 0;
    M669_Volume[channel]          = 16;
}

void M669_GetNewNote(uint32_t channel)
{
    if (M669_SampleNr[channel] != 0) {
        if (M669_Note[channel] >= 64u) {
            M669_Period[channel]      = 0;
            M669_ChannelActive[channel] = 0;
        } else {
            M669_Period[channel]      = M669_GlissPeriod[channel] =
                Calc_669period(M669_Note[channel], M669_SampleFinetune[channel]);
            M669_GlissFlag[channel]    = 0;
            M669_SamplePosition[channel]  = 0;
            M669_SampleFraction[channel]  = 0;
            M669_ChannelActive[channel]    = 1;
        }
    } else {
        M669_ChannelActive[channel] = 0;
    }
}

/* ---- mixer handoff ---- */

static void M669_to_Mixer(void)
{
    uint32_t channel_index;

    g_ChannelLast = M669_CHANNELS;

    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        if (M669_Period[channel_index] < 10u) M669_ChannelActive[channel_index] = 0;
        g_ChannelActive[channel_index]   = M669_ChannelActive[channel_index];
        g_ChannelVolume[channel_index]  = (uint32_t)M669_Volume[channel_index] << 4;
        g_ChannelPanning[channel_index] = (uint32_t)M669_Panning[channel_index];

        if (M669_ChannelActive[channel_index]) {
            g_ChannelSamplePosition[channel_index]  = M669_SamplePosition[channel_index];
            g_ChannelSampleFraction[channel_index]  = M669_SampleFraction[channel_index];
            M669_Frequency[channel_index]           = Calc_669frequency(channel_index);
            g_ChannelSampleFrequency[channel_index] = (M669_Frequency[channel_index] + 2u) >> 2;

            if (M669_SampleNr[channel_index] && g_ChannelSampleNr[channel_index] != M669_SampleNr[channel_index]) {
                g_ChannelSampleNr[channel_index]      = M669_SampleNr[channel_index];
                g_ChannelSampleBits[channel_index]    = 8;
                g_ChannelSampleMode[channel_index]    = 0;
                g_ChannelSampleAddress[channel_index] = M669_SampleAddress[channel_index];
                g_ChannelSampleLength[channel_index]  = M669_SampleLength[channel_index];
                g_ChannelLoopMode[channel_index]      = M669_SampleLoop[channel_index];
                g_ChannelLoopBegin[channel_index]     = M669_SampleLoopBegin[channel_index];
                g_ChannelLoopEnd[channel_index]       = M669_SampleLoopEnd[channel_index];
            }
        }
    }
}

static void Mixer_to_M669(void)
{
    uint32_t channel_index;
    for (channel_index = 0; channel_index < M669_CHANNELS; channel_index++) {
        if (M669_ChannelActive[channel_index]) {
            M669_SamplePosition[channel_index]  = g_ChannelSamplePosition[channel_index];
            M669_SampleFraction[channel_index]  = g_ChannelSampleFraction[channel_index];
            M669_ChannelActive[channel_index]    = (uint8_t)g_ChannelActive[channel_index];
        }
    }
}

/* ---- render block (interrupt_669 port) ---- */

void m669_render_block(uint32_t block_frames)
{
    uint32_t toplay;

    M669_buffer_rest = block_frames;
    g_MixerAddress   = 0;
    mixer_clear();

    while (M669_buffer_rest > 0) {
        if (M669_tick_rest <= M669_buffer_rest) {
            toplay = M669_tick_rest;

            M669_to_Mixer();
            mixer_do_pre_mixing(toplay);
            Mixer_to_M669();

            g_MixerAddress += toplay;
            M669_tick++;

            if (M669_tick == M669_speed) {
                M669_tick = 0;
                M669_goRowOrder();
                M669_unpack_row(M669_Pattern, M669_row);
                if (s_dat_ready == 1) M669_read_row();
            } else {
                M669_TickEffect();
            }

            M669_buffer_rest -= toplay;
            M669_tick_rest    = M669_TickLength;
        } else {
            toplay = M669_buffer_rest;

            M669_to_Mixer();
            mixer_do_pre_mixing(toplay);
            Mixer_to_M669();

            g_MixerAddress   += toplay;
            M669_tick_rest   -= toplay;
            M669_buffer_rest  = 0;
        }
    }
}
