#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "effect/efc_mod.h"
#include "format/mod.h"
#include "mixer/mixer_scalar.h"
#include "util/calc.h"

/* ---- period table [16 finetunes][84 notes] — filled at load time ---- */

/*
 * Seed values: octaves 1-3 only (indices 24..59 = notes 12..47 in the table).
 * load_MOD extends to full 7-octave range (indices 0..83).
 * Layout: [0..11]=oct0, [12..23]=oct1, [24..35]=oct2(seed),
 *          [36..47]=oct3(seed), [48..59]=oct4(seed),
 *          [60..71]=oct5, [72..83]=oct6
 */
uint32_t MOD_Periods[16][84] = {
  /* Tuning 0 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339, 320,
    302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +1 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337, 318,
    300, 284, 268, 253, 239, 225, 213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +2 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335, 316,
    298, 282, 266, 251, 237, 224, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +3 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332, 314,
    296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +4 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441, 416, 392, 370, 350, 330, 312,
    294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +5 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328, 309,
    292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +6 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325, 307,
    290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning +7 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323, 305,
    288, 272, 256, 242, 228, 216, 204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -8 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339,
    320, 302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -7 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337,
    318, 300, 284, 268, 253, 238, 225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -6 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335,
    316, 298, 282, 266, 251, 237, 223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -5 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332,
    314, 296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -4 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467, 441, 416, 392, 370, 350, 330,
    312, 294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -3 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328,
    309, 292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -2 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325,
    307, 290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  /* Tuning -1 */
  { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323,
    305, 288, 272, 256, 242, 228, 216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } };

/* Known 31-sample MOD IDs [8] and their channel counts */
static const char MOD_id[8][5]         = { "M.K.", "FLT4", "FLT6", "FLT8", "4CHN", "6CHN", "8CHN", "M!K!" };
static const uint32_t MOD_channelNr[8] = { 4, 4, 6, 8, 4, 6, 8, 4 };

/* Song state */
uint32_t MOD_channels = 4;
uint32_t MOD_samples  = 15;
uint32_t MOD_patterns = 0;
uint8_t  MOD_OrderNum  = 0;

/* Runtime state */
uint8_t MOD_RowBuffer[MOD_MAXCHANNELS * 4];

uint32_t MOD_LastPattern = 0xFFFFu;
uint32_t MOD_LastRow     = 0xFFFFu;
uint32_t MOD_TickLength  = 0;

uint32_t MOD_row = 0, MOD_jump = 0, MOD_nextrow = 0, MOD_nextorder = 0;
uint32_t MOD_tick = 0, MOD_Order = 0, MOD_Pattern = 0;
uint32_t MOD_buffer_rest = 0, MOD_tick_rest = 0;

uint8_t MOD_flag_amigalimits = 0;
uint8_t MOD_flag_note        = 0;
uint8_t MOD_tempo            = 128;
uint8_t MOD_speed            = 6;

uint8_t   MOD_ChannelActive[MOD_MAXCHANNELS];
uint8_t   MOD_SampleNr[MOD_MAXCHANNELS];
uintptr_t MOD_SampleAddress[MOD_MAXCHANNELS];
uint32_t  MOD_SampleLength[MOD_MAXCHANNELS];
uint32_t  MOD_SamplePosition[MOD_MAXCHANNELS];
uint32_t  MOD_SampleFraction[MOD_MAXCHANNELS];
uint8_t   MOD_SampleLoop[MOD_MAXCHANNELS];
uint32_t  MOD_SampleLoopBegin[MOD_MAXCHANNELS];
uint32_t  MOD_SampleLoopEnd[MOD_MAXCHANNELS];
uint32_t  MOD_SampleFinetune[MOD_MAXCHANNELS];

uint8_t MOD_Volume[MOD_MAXCHANNELS];
uint8_t MOD_Panning[MOD_MAXCHANNELS];
uint8_t MOD_Note[MOD_MAXCHANNELS];
uint8_t MOD_Effect[MOD_MAXCHANNELS];
uint8_t MOD_EffectInfo[MOD_MAXCHANNELS];
uint8_t MOD_LastEffect[MOD_MAXCHANNELS];
uint8_t MOD_LastEffectInfo[MOD_MAXCHANNELS];

uint32_t MOD_Period[MOD_MAXCHANNELS];
int32_t  MOD_PeriodAdjust[MOD_MAXCHANNELS];
uint32_t MOD_Frequency[MOD_MAXCHANNELS];

/* ---- internal workspace pointers ---- */
static uint8_t*  s_buf       = NULL;
static uint8_t** s_ins       = NULL;
static uint8_t** s_smp       = NULL;
static uint8_t** s_pat       = NULL;
static uint8_t*  s_order     = NULL;
static uint8_t   s_dat_ready = 0;
static uint32_t s_song_loops = 0;
static char s_title[21]      = { 0 };

/* ---- helpers ---- */

static uint16_t be16( const uint8_t* p ) { return (uint16_t)( (uint32_t)p[0] << 8 | p[1] ); }

/* ---- workspace sizing ---- */

/*
 * We use worst-case: 31 samples, 128 patterns.
 * This avoids parsing the full orderlist in sizing.
 */
size_t mod_workspace_bytes( const uint8_t* data, size_t size )
{
  size_t ptr_align, ws;
  (void)data;
  if ( size < 600u )
    return 0;
  ptr_align = sizeof( uint8_t* ) - 1u;
  ws        = size;
  ws        = ( ws + ptr_align ) & ~ptr_align;
  ws += (size_t)MOD_MAXSAMPLES * sizeof( uint8_t* ) * 2u; /* ins + smp */
  ws += (size_t)MOD_MAXPATTERNS * sizeof( uint8_t* );     /* pat */
  ws += 65544u;                                           /* MasterVolumeTable */
  ws += 128u;                                             /* alignment overhead */
  return ws;
}

/* ---- loader ---- */

int mod_load( const uint8_t* data, size_t size, uint8_t* ws, size_t ws_size )
{
  uint8_t* p;
  size_t ptr_align, module_aligned;
  uint32_t i, t, channel_index, value;
  uint32_t smpcount, channels;
  uint32_t base_pat, base_smp;

  s_dat_ready  = 0;
  s_song_loops = 0;

  if ( size < 600u )
    return -1;
  if ( ws_size < mod_workspace_bytes( data, size ) )
    return -2;

  /* carve workspace */
  ptr_align      = sizeof( uint8_t* ) - 1u;
  module_aligned = ( size + ptr_align ) & ~ptr_align;

  s_buf = ws;
  memcpy( s_buf, data, size );

  memcpy( s_title, s_buf, 20 );
  s_title[20] = '\0';

  p     = ws + module_aligned;
  s_ins = (uint8_t**)p;
  p += (size_t)MOD_MAXSAMPLES * sizeof( uint8_t* );
  s_smp = (uint8_t**)p;
  p += (size_t)MOD_MAXSAMPLES * sizeof( uint8_t* );
  s_pat = (uint8_t**)p;
  p += (size_t)MOD_MAXPATTERNS * sizeof( uint8_t* );
  p                  = (uint8_t*)( ( (uintptr_t)p + 7u ) & ~(uintptr_t)7u );
  g_master_vol_table = (int32_t*)p + 8192;

  /* extend period table: octave 0, 5, 6 from seeded octaves 1-3 */
  for ( t = 0; t < 16; t++ )
  {
    for ( i = 0; i < 12; i++ )
    {
      MOD_Periods[t][i]      = MOD_Periods[t][i + 24] * 4u;
      MOD_Periods[t][i + 12] = MOD_Periods[t][i + 24] * 2u;
      MOD_Periods[t][i + 60] = ( MOD_Periods[t][i + 48] + 1u ) / 2u;
      MOD_Periods[t][i + 72] = ( MOD_Periods[t][i + 48] + 2u ) / 4u;
    }
  }

  /* detect format from ID at offset 1080 */
  channels = 4;
  smpcount = 15;
  base_pat = 600u;
  base_smp = 600u;

  if ( size >= 1084u )
  {
    const uint8_t* id = s_buf + 1080u;
    for ( t = 0; t < 8; t++ )
    {
      if ( memcmp( id, MOD_id[t], 4 ) == 0 )
      {
        channels = MOD_channelNr[t];
        smpcount = 31;
        base_pat = 1084u;
        base_smp = 1084u;
        break;
      }
    }
  }

  MOD_channels = channels;
  MOD_samples  = smpcount;

  /* order list */
  if ( smpcount == 15 )
  {
    MOD_OrderNum = s_buf[470];
    s_order      = s_buf + 472u;
  }
  else
  {
    MOD_OrderNum = s_buf[950];
    s_order      = s_buf + 952u;
  }

  /* find highest pattern index to determine pattern count */
  value = 0;
  for ( i = 0; i < 127u; i++ )
  {
    if ( s_order[i] > value )
      value = s_order[i];
  }
  MOD_patterns = value + 1u;

  /* byte-swap 16-bit big-endian fields in instrument blocks */
  {
    uint8_t* ptr = s_buf + 20u;
    for ( i = 0; i < smpcount; i++ )
    {
      uint8_t* ins = ptr + i * 30u;
      uint16_t sample_length, loop_start, loop_length;

      ins[21] = 0; /* terminate sample name */

      sample_length = be16( ins + 22 );
      loop_start    = be16( ins + 26 );
      loop_length   = be16( ins + 28 );

      /* write back LE */
      ins[22] = (uint8_t)( sample_length & 0xFF );
      ins[23] = (uint8_t)( sample_length >> 8 );
      ins[26] = (uint8_t)( loop_start & 0xFF );
      ins[27] = (uint8_t)( loop_start >> 8 );
      ins[28] = (uint8_t)( loop_length & 0xFF );
      ins[29] = (uint8_t)( loop_length >> 8 );

      /* clamp loop boundaries */
      if ( loop_start > sample_length )
      {
        loop_start  = 0;
        loop_length = 0;
        ins[26] = ins[27] = ins[28] = ins[29] = 0;
      }
      if ( (uint32_t)loop_start + loop_length > sample_length )
      {
        uint16_t newlen = sample_length - loop_start;
        ins[28]         = (uint8_t)( newlen & 0xFF );
        ins[29]         = (uint8_t)( newlen >> 8 );
      }
    }
  }

  /* build instrument and sample pointers */
  {
    uint8_t* ptr = s_buf + 20u;
    /* sample data starts after header + all patterns */
    value = base_smp + MOD_patterns * channels * 64u * 4u;

    for ( i = 0; i < smpcount; i++ )
    {
      uint8_t* ins = ptr + i * 30u;
      uint32_t sample_length;
      s_ins[i]      = ins;
      s_smp[i]      = s_buf + value;
      sample_length = (uint32_t)( *(uint16_t*)( ins + 22 ) ) * 2u;
      value += sample_length;
      if ( value > size )
        return -1;
    }
  }

  /* build pattern pointers */
  for ( i = 0; i < MOD_patterns; i++ )
  {
    s_pat[i] = s_buf + base_pat + i * channels * 64u * 4u;
  }

  /* init song state */
  MOD_initvariables();
  MOD_initEffects();

  /* master volume: scale by channel count */
  g_MasterVolume = (uint32_t)( ( 0x50u - channels * 4u ) * 256u );

  MOD_flag_amigalimits = 0;
  MOD_tempo            = 128;
  MOD_speed            = 6;

  /* default panning: LRRL repeating (channel_index%4: 0=L, 1=R, 2=R, 3=L) */
  for ( channel_index = 0; channel_index < channels; channel_index++ )
  {
    uint32_t panning_side      = channel_index & 3u;
    MOD_Panning[channel_index] = ( panning_side == 1u || panning_side == 2u ) ? 100u : 28u;
  }

  MOD_TickLength = s3m_calc_speed( MOD_tempo, g_MixSpeed );
  MOD_tick_rest  = MOD_TickLength;
  MOD_row        = 0;
  MOD_jump       = 0;
  MOD_tick       = 0;
  MOD_Order      = 0;
  MOD_Pattern    = s_order[0];

  mixer_calc_master_vol32( g_MasterVolume, g_master_vol_table );

  MOD_unpack_row( MOD_Pattern, MOD_row );
  MOD_read_row();

  s_dat_ready = 1;
  return 0;
}

void mod_close( void )
{
  s_dat_ready  = 0;
  s_song_loops = 0;
  s_buf        = NULL;
  s_ins = s_smp = s_pat = NULL;
  s_order               = NULL;
  g_master_vol_table    = NULL;
}

uint8_t mod_is_done( void ) { return s_dat_ready == 2u; }
uint32_t mod_song_loops( void ) { return s_song_loops; }
const char* mod_song_title( void ) { return s_title; }
void mod_mark_looped( void ) { s_song_loops++; }
void mod_restart( void )
{
  if ( s_dat_ready == 2 )
    s_dat_ready = 1;
}

/* ---- initialisation ---- */

void MOD_initvariables( void )
{
  uint32_t channel_index;

  MOD_LastPattern = 0xFFFFu;
  MOD_LastRow     = 0xFFFFu;
  MOD_TickLength  = 0;
  MOD_row = MOD_jump = MOD_nextrow = MOD_nextorder = 0;
  MOD_tick = MOD_Order = MOD_Pattern = 0;
  MOD_buffer_rest = MOD_tick_rest = 0;
  MOD_flag_amigalimits            = 0;
  MOD_flag_note                   = 0;
  MOD_tempo                       = 128;
  MOD_speed                       = 6;

  for ( channel_index = 0; channel_index < MOD_channels; channel_index++ )
  {
    MOD_ChannelActive[channel_index]    = 0;
    MOD_SampleNr[channel_index]        = 0;
    MOD_SampleAddress[channel_index]   = 0;
    MOD_SampleLength[channel_index]    = 0;
    MOD_SamplePosition[channel_index]  = 0;
    MOD_SampleFraction[channel_index]  = 0;
    MOD_SampleLoop[channel_index]      = 0;
    MOD_SampleLoopBegin[channel_index] = 0;
    MOD_SampleLoopEnd[channel_index]   = 0;
    MOD_SampleFinetune[channel_index]  = 0;
    MOD_Volume[channel_index]          = 0;
    MOD_Panning[channel_index]         = 0x40u;
    MOD_Note[channel_index]            = 0xFFu;
    MOD_Effect[channel_index]          = 0xFFu;
    MOD_EffectInfo[channel_index]      = 0;
    MOD_LastEffect[channel_index]      = 0xFFu;
    MOD_LastEffectInfo[channel_index]  = 0;
    MOD_Period[channel_index]         = 0;
    MOD_PeriodAdjust[channel_index]   = 0;
    MOD_Frequency[channel_index]       = 0;
  }
}

/* ---- pattern decode ---- */

static void mod_clear_row( void )
{
  uint32_t i;
  for ( i = 0; i < MOD_MAXCHANNELS * 4u; i += 4u )
  {
    MOD_RowBuffer[i + 0] = 0xFFu;
    MOD_RowBuffer[i + 1] = 0x00u;
    MOD_RowBuffer[i + 2] = 0xFFu;
    MOD_RowBuffer[i + 3] = 0x00u;
  }
}

void MOD_unpack_row( uint32_t pat_nr, uint32_t row_nr )
{
  uint32_t channel_index;
  const uint8_t* p;

  if ( pat_nr >= MOD_patterns )
  {
    mod_clear_row();
    MOD_LastPattern = pat_nr;
    MOD_LastRow     = row_nr;
    return;
  }

  if ( pat_nr == MOD_LastPattern && row_nr == MOD_LastRow )
    return;

  mod_clear_row();

  p = s_pat[pat_nr] + row_nr * MOD_channels * 4u;

  for ( channel_index = 0; channel_index < MOD_channels; channel_index++ )
  {
    /* 4-byte MOD cell: [s_hi|p_hi] [p_lo] [s_lo|e] [e_inf] */
    uint32_t period = ( ( (uint32_t)p[0] & 0x0Fu ) << 8 ) | p[1];
    uint32_t note    = 0xFFu;
    uint32_t period_slot, val;

    if ( period != 0 )
    {
      int32_t best_dist = 0x7FFFFFFFl;
      int32_t dist;
      note = 0;
      for ( period_slot = 0; period_slot < 84u; period_slot++ )
      {
        if ( MOD_Periods[0][period_slot] == 0 )
          continue;
        dist = (int32_t)period - (int32_t)MOD_Periods[0][period_slot];
        if ( dist < 0 )
          dist = -dist;
        if ( dist < best_dist )
        {
          best_dist = dist;
          note      = period_slot;
        }
      }
    }
    MOD_RowBuffer[channel_index * 4 + 0] = (uint8_t)note;

    val = ( (uint32_t)p[0] & 0xF0u ) | ( (uint32_t)p[2] >> 4 );
    if ( val > MOD_samples )
      val = 0;
    MOD_RowBuffer[channel_index * 4 + 1] = (uint8_t)val;

    val                                  = (uint32_t)p[2] & 0x0Fu;
    MOD_RowBuffer[channel_index * 4 + 2] = (uint8_t)val;

    val                                  = (uint32_t)p[3];
    MOD_RowBuffer[channel_index * 4 + 3] = (uint8_t)val;

    if ( MOD_RowBuffer[channel_index * 4 + 2] == 0 && MOD_RowBuffer[channel_index * 4 + 3] == 0 )
      MOD_RowBuffer[channel_index * 4 + 2] = 0xFFu;

    p += 4u;
  }

  MOD_LastPattern = pat_nr;
  MOD_LastRow     = row_nr;
}

/* ---- period / frequency helpers ---- */

uint32_t Calc_AMIGAperiod( uint32_t note, uint32_t finetune ) { return MOD_Periods[finetune][note] << 4u; /* 4-bit fixed-point */ }

static uint32_t Calc_AMIGAfrequency( uint32_t channel )
{
  uint32_t p = MOD_Period[channel] + (uint32_t)MOD_PeriodAdjust[channel];
  if ( p == 0 )
    return 0;
  /* freq = 7159090.5 / (period*2); period extended by 4 bits → /4; kept at /4 */
  return ( 14318181UL << 4u ) / p;
}

/* ---- order / row navigation ---- */

void MOD_goRowOrder( void )
{
  if ( s_dat_ready != 1 )
    return;

  if ( MOD_jump == 0 )
  {
    if ( MOD_flag_note == 0 )
      MOD_row++;
    if ( MOD_row == 64u )
    {
      MOD_Order++;
      MOD_row = 0;
      if ( MOD_Order >= MOD_OrderNum )
      {
        MOD_Order = 0;
        s_song_loops++;
        s_dat_ready = 2;
      }
      MOD_Pattern = s_order[MOD_Order];
    }
  }
  else
  {
    MOD_jump = 0;
    if ( MOD_nextrow <= 63u && MOD_nextorder < MOD_OrderNum )
    {
      MOD_row     = MOD_nextrow;
      MOD_Order   = MOD_nextorder;
      MOD_Pattern = s_order[MOD_Order];
    }
    MOD_nextrow   = 0;
    MOD_nextorder = 0;
  }
}

/* ---- sample / note triggers ---- */

void MOD_GetNewSample( uint32_t channel )
{
  uint32_t sn, len, lstart, llen;
  uint8_t* ins;

  if ( MOD_SampleNr[channel] == 0 )
    return;
  sn = MOD_SampleNr[channel] - 1u;
  if ( sn >= MOD_samples )
  {
    MOD_SampleNr[channel] = 0;
    return;
  }

  ins = s_ins[sn];
  len = (uint32_t)( *(uint16_t*)( ins + 22 ) ) * 2u;
  if ( len == 0 )
  {
    MOD_SampleNr[channel] = 0;
    return;
  }

  lstart = (uint32_t)( *(uint16_t*)( ins + 26 ) ) * 2u;
  llen   = (uint32_t)( *(uint16_t*)( ins + 28 ) ) * 2u;

  MOD_SampleAddress[channel]   = (uintptr_t)s_smp[sn];
  MOD_SampleLength[channel]    = len;
  MOD_SampleLoopBegin[channel] = lstart;
  MOD_SampleLoopEnd[channel]   = lstart + llen;
  MOD_SampleLoop[channel]      = ( llen > 2u ) ? 1u : 0u;
  MOD_SampleFinetune[channel]  = (uint32_t)( ins[24] & 0x0Fu );

  MOD_TremoloVolume[channel] = MOD_Volume[channel] = ins[25];
}

void MOD_GetNewNote( uint32_t channel )
{
  if ( MOD_SampleNr[channel] != 0 )
  {
    if ( MOD_Note[channel] >= 84u )
    {
      MOD_Period[channel]      = 0;
      MOD_ChannelActive[channel] = 0;
    }
    else
    {
      MOD_Period[channel] = MOD_GlissPeriod[channel] = Calc_AMIGAperiod( MOD_Note[channel], MOD_SampleFinetune[channel] );
      MOD_SamplePosition[channel]                      = 0;
      MOD_SampleFraction[channel]                      = 0;
      MOD_ChannelActive[channel]                        = 1;
    }
  }
  else
  {
    MOD_ChannelActive[channel] = 0;
  }
}

/* ---- mixer handoff ---- */

static void MOD_to_Mixer( void )
{
  uint32_t channel_index;

  g_ChannelLast = MOD_channels;

  for ( channel_index = 0; channel_index < MOD_channels; channel_index++ )
  {
    if ( MOD_Period[channel_index] < 10u )
      MOD_ChannelActive[channel_index] = 0;
    g_ChannelActive[channel_index]   = MOD_ChannelActive[channel_index];
    g_ChannelVolume[channel_index]  = (uint32_t)MOD_Volume[channel_index] << 2;
    g_ChannelPanning[channel_index] = (uint32_t)MOD_Panning[channel_index] * 2u;

    if ( MOD_ChannelActive[channel_index] )
    {
      g_ChannelSamplePosition[channel_index]  = MOD_SamplePosition[channel_index];
      g_ChannelSampleFraction[channel_index]  = MOD_SampleFraction[channel_index];
      MOD_Frequency[channel_index]            = Calc_AMIGAfrequency( channel_index );
      g_ChannelSampleFrequency[channel_index] = ( MOD_Frequency[channel_index] + 2u ) >> 2;

      if ( MOD_SampleNr[channel_index] && g_ChannelSampleNr[channel_index] != MOD_SampleNr[channel_index] )
      {
        g_ChannelSampleNr[channel_index]      = MOD_SampleNr[channel_index];
        g_ChannelSampleBits[channel_index]    = 8;
        g_ChannelSampleMode[channel_index]    = 0;
        g_ChannelSampleAddress[channel_index] = MOD_SampleAddress[channel_index];
        g_ChannelSampleLength[channel_index]  = MOD_SampleLength[channel_index];
        g_ChannelLoopMode[channel_index]      = MOD_SampleLoop[channel_index];
        g_ChannelLoopBegin[channel_index]     = MOD_SampleLoopBegin[channel_index];
        g_ChannelLoopEnd[channel_index]       = MOD_SampleLoopEnd[channel_index];
      }
    }
  }
}

static void Mixer_to_MOD( void )
{
  uint32_t channel_index;
  for ( channel_index = 0; channel_index < MOD_channels; channel_index++ )
  {
    if ( MOD_ChannelActive[channel_index] )
    {
      MOD_SamplePosition[channel_index] = g_ChannelSamplePosition[channel_index];
      MOD_SampleFraction[channel_index] = g_ChannelSampleFraction[channel_index];
      MOD_ChannelActive[channel_index]   = (uint8_t)g_ChannelActive[channel_index];
    }
  }
}

/* ---- render block (interrupt_MOD port) ---- */

void mod_render_block( uint32_t block_frames )
{
  uint32_t toplay;

  MOD_buffer_rest = block_frames;
  g_MixerAddress  = 0;
  mixer_clear();

  while ( MOD_buffer_rest > 0 )
  {
    if ( MOD_tick_rest <= MOD_buffer_rest )
    {
      toplay = MOD_tick_rest;

      MOD_to_Mixer();
      mixer_do_pre_mixing( toplay );
      Mixer_to_MOD();

      g_MixerAddress += toplay;
      MOD_tick++;

      if ( MOD_tick == MOD_speed )
      {
        MOD_tick = 0;
        MOD_goRowOrder();
        MOD_unpack_row( MOD_Pattern, MOD_row );
        if ( s_dat_ready == 1 )
          MOD_read_row();
      }
      else
      {
        MOD_TickEffect();
      }

      MOD_buffer_rest -= toplay;
      MOD_tick_rest = MOD_TickLength;
    }
    else
    {
      toplay = MOD_buffer_rest;

      MOD_to_Mixer();
      mixer_do_pre_mixing( toplay );
      Mixer_to_MOD();

      g_MixerAddress += toplay;
      MOD_tick_rest -= toplay;
      MOD_buffer_rest = 0;
    }
  }
}
