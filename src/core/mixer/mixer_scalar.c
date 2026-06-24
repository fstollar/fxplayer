#include "mixer/mixer_scalar.h"
#include "util/calc.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---- master volume table pointer (set by each format loader) ---- */
int32_t* g_master_vol_table = NULL;

/* ---- global mixer state ---- */

uint32_t g_MixSpeed          = 48000;
uint32_t g_MasterVolume      = 0;
uint32_t g_flag_stereo       = 1;
uint32_t g_flag_interpolate  = 1;
uint32_t g_flag_soft_clip    = 1;
uint32_t g_ChannelSeperation = 256;
uint32_t g_GlobalVolume      = 256;

uint32_t g_MixerAddress = 0;
uint32_t g_ChannelUsed  = 0;
uint32_t g_ChannelLast  = 0;

uint32_t  g_ChannelActiv[FX_MAXCHANNELS];
uint32_t  g_ChannelFlag[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleNr[FX_MAXCHANNELS];
uint32_t  g_ChannelVolume[FX_MAXCHANNELS];
uint32_t  g_ChannelPanning[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleBits[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleMode[FX_MAXCHANNELS];
uintptr_t g_ChannelSampleAddress[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleLength[FX_MAXCHANNELS];
uint32_t  g_ChannelLoopMode[FX_MAXCHANNELS];
uint32_t  g_ChannelLoopBegin[FX_MAXCHANNELS];
uint32_t  g_ChannelLoopEnd[FX_MAXCHANNELS];
uint32_t  g_ChannelSamplePosition[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleFraction[FX_MAXCHANNELS];
uint32_t  g_ChannelSampleFrequence[FX_MAXCHANNELS];
int32_t   g_ChannelLastValueLeft[FX_MAXCHANNELS];
int32_t   g_ChannelLastValueRight[FX_MAXCHANNELS];
uint32_t  g_ChannelVolumeLeft[FX_MAXCHANNELS];
uint32_t  g_ChannelVolumeRight[FX_MAXCHANNELS];
int32_t   g_DeltaSamplePosition[FX_MAXCHANNELS];
int32_t   g_DeltaSampleFraction[FX_MAXCHANNELS];
uint32_t  g_NextSamplePosition[FX_MAXCHANNELS];
uint32_t  g_NextSampleFraction[FX_MAXCHANNELS];
uint32_t  g_NextChannelActiv[FX_MAXCHANNELS];

/* Static 32-bit mix scratch: stereo * FX_INTERNAL_BLOCK int32_t values */
static int32_t s_mix_scratch[FX_INTERNAL_BLOCK * 2];

/* ---- reset / clear ---- */

void mixer_reset( void )
{
  uint32_t channel_index;
  g_ChannelUsed = 0;
  g_ChannelLast = 0;
  for ( channel_index = 0; channel_index < FX_MAXCHANNELS; channel_index++ )
  {
    g_ChannelActiv[channel_index]           = 0;
    g_ChannelFlag[channel_index]            = 0;
    g_ChannelSampleNr[channel_index]        = 0;
    g_ChannelVolume[channel_index]          = 0;
    g_ChannelPanning[channel_index]         = 128;
    g_ChannelSampleBits[channel_index]      = 0;
    g_ChannelSampleMode[channel_index]      = 0;
    g_ChannelSampleAddress[channel_index]   = 0;
    g_ChannelSampleLength[channel_index]    = 0;
    g_ChannelSamplePosition[channel_index]  = 0;
    g_ChannelSampleFraction[channel_index]  = 0;
    g_ChannelSampleFrequence[channel_index] = 0;
    g_ChannelLoopMode[channel_index]        = 0;
    g_ChannelLoopBegin[channel_index]       = 0;
    g_ChannelLoopEnd[channel_index]         = 0;
    g_ChannelVolumeLeft[channel_index]      = 0;
    g_ChannelVolumeRight[channel_index]     = 0;
    g_DeltaSamplePosition[channel_index]    = 0;
    g_DeltaSampleFraction[channel_index]    = 0;
    g_NextSamplePosition[channel_index]     = 0;
    g_NextSampleFraction[channel_index]     = 0;
    g_NextChannelActiv[channel_index]       = 0;
  }
}

void mixer_clear( void )
{
  /* 32-bit mix buffer: zero (matching clearMix32 in original) */
  memset( s_mix_scratch, 0, sizeof( s_mix_scratch ) );
}

/* ---- scaling calculation ---- */

static uint32_t calc_scaling( uint32_t channel )
{
  s3m_divide_64bit( g_ChannelSampleFrequence[channel] << 1, 0, 0, g_MixSpeed );
  return ( g_division + 1u ) >> 1;
}

/* ---- mixing inner loops ---- */

/*
 * Non-interpolated, 32-bit accumulation buffer.
 * Volume scaling matches original ASM tables:
 *   8-bit:  mix[i] += (int8_t)s8[pos] * vol * 256
 *   16-bit: mix[i] += (int16_t)s16[pos] * vol
 * The MasterVol path divides by 0x800000 = 8388608.
 */
static void mix32_channel( uint32_t channel_index, uint32_t count, uint32_t mix_pos )
{
  uint32_t vol, vol_l, vol_r;
  uint32_t pos, frac;
  int32_t dpos, dfrac;
  uint32_t i;
  int32_t* mb;

  if ( count == 0 )
    return;
  vol = ( g_ChannelVolume[channel_index] * g_GlobalVolume + 128u ) >> 8;
  if ( vol == 0 )
    return;

  pos   = g_ChannelSamplePosition[channel_index];
  frac  = g_ChannelSampleFraction[channel_index];
  dpos  = g_DeltaSamplePosition[channel_index];
  dfrac = g_DeltaSampleFraction[channel_index];

  if ( g_flag_stereo )
  {
    vol_l = ( g_ChannelVolumeLeft[channel_index] * g_GlobalVolume + 128u ) >> 8;
    vol_r = ( g_ChannelVolumeRight[channel_index] * g_GlobalVolume + 128u ) >> 8;
  }
  else
  {
    vol_l = vol_r = vol;
  }

  mb = s_mix_scratch + ( g_flag_stereo ? mix_pos * 2u : mix_pos );

  if ( g_ChannelSampleBits[channel_index] == 8 )
  {
    const uint8_t* s8 = (const uint8_t*)g_ChannelSampleAddress[channel_index];

    if ( !g_flag_stereo )
    {
      for ( i = 0; i < count; i++ )
      {
        mb[i] += (int32_t)(int8_t)s8[pos] * (int32_t)vol * 256;
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
    else
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s = (int32_t)(int8_t)s8[pos] * 256;
        mb[i * 2 + 0] += s * (int32_t)vol_l;
        mb[i * 2 + 1] += s * (int32_t)vol_r;
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
  }
  else
  { /* 16-bit */
    const int16_t* s16 = (const int16_t*)g_ChannelSampleAddress[channel_index];
    if ( !g_flag_stereo )
    {
      for ( i = 0; i < count; i++ )
      {
        mb[i] += (int32_t)s16[pos] * (int32_t)vol;
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
    else
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s = s16[pos];
        mb[i * 2 + 0] += s * (int32_t)vol_l;
        mb[i * 2 + 1] += s * (int32_t)vol_r;
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
  }
}

/*
 * Interpolated 32-bit path (safe segment: pos+1 guaranteed < length).
 * Scaling matches mixr_32i.asm:
 *   8-bit:  interp = (s0<<16) + (s1-s0)*frac; mix[i] += interp * vol >> 8
 *   16-bit: interp = (s0<<8)  + (s1-s0)*frac/256; mix[i] += interp * vol >> 8
 */
static void mix32_ichannel( uint32_t channel_index, uint32_t count, uint32_t mix_pos )
{
  uint32_t vol, vol_l, vol_r;
  uint32_t pos, frac;
  int32_t dpos, dfrac;
  uint32_t i;
  int32_t* mb;

  if ( count == 0 )
    return;
  vol = ( g_ChannelVolume[channel_index] * g_GlobalVolume + 128u ) >> 8;
  if ( vol == 0 )
    return;

  pos   = g_ChannelSamplePosition[channel_index];
  frac  = g_ChannelSampleFraction[channel_index];
  dpos  = g_DeltaSamplePosition[channel_index];
  dfrac = g_DeltaSampleFraction[channel_index];

  if ( g_flag_stereo )
  {
    vol_l = ( g_ChannelVolumeLeft[channel_index] * g_GlobalVolume + 128u ) >> 8;
    vol_r = ( g_ChannelVolumeRight[channel_index] * g_GlobalVolume + 128u ) >> 8;
  }
  else
  {
    vol_l = vol_r = vol;
  }

  mb = s_mix_scratch + ( g_flag_stereo ? mix_pos * 2u : mix_pos );

  if ( g_ChannelSampleBits[channel_index] == 8 )
  {
    const uint8_t* s8 = (const uint8_t*)g_ChannelSampleAddress[channel_index];
    if ( !g_flag_stereo )
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s0     = (int8_t)s8[pos];
        int32_t s1     = (int8_t)s8[pos + 1];
        int32_t interp = (int32_t)( (uint32_t)s0 << 16 ) + ( s1 - s0 ) * (int32_t)frac;
        mb[i] += (int32_t)( (int64_t)interp * vol >> 8 );
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
    else
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s0     = (int8_t)s8[pos];
        int32_t s1     = (int8_t)s8[pos + 1];
        int32_t interp = (int32_t)( (uint32_t)s0 << 16 ) + ( s1 - s0 ) * (int32_t)frac;
        mb[i * 2 + 0] += (int32_t)( (int64_t)interp * vol_l >> 8 );
        mb[i * 2 + 1] += (int32_t)( (int64_t)interp * vol_r >> 8 );
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
  }
  else
  {
    const int16_t* s16 = (const int16_t*)g_ChannelSampleAddress[channel_index];
    if ( !g_flag_stereo )
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s0     = s16[pos];
        int32_t s1     = s16[pos + 1];
        int32_t interp = ( s0 << 8 ) + ( ( ( s1 - s0 ) * (int32_t)frac ) >> 8 );
        mb[i] += (int32_t)( (int64_t)interp * vol >> 8 );
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
    else
    {
      for ( i = 0; i < count; i++ )
      {
        int32_t s0     = s16[pos];
        int32_t s1     = s16[pos + 1];
        int32_t interp = ( s0 << 8 ) + ( ( ( s1 - s0 ) * (int32_t)frac ) >> 8 );
        mb[i * 2 + 0] += (int32_t)( (int64_t)interp * vol_l >> 8 );
        mb[i * 2 + 1] += (int32_t)( (int64_t)interp * vol_r >> 8 );
        frac = (uint32_t)( (int32_t)frac + dfrac );
        pos  = (uint32_t)( (int32_t)pos + dpos ) + ( frac >> 16 );
        frac &= 0xFFFFu;
      }
    }
  }
}

/*
 * Single-sample interpolated write for the "carry" segment at loop/end boundaries.
 * Translates Mix32ICarry from mixer.cpp (already in C in the original).
 */
static void mix32_icarry( uint32_t channel_index, uint32_t pos1, uint32_t pos2, uint32_t fraction, uint32_t mix_pos )
{
  uint32_t vol;
  int32_t s1, s2, interp;

  vol = ( g_ChannelVolume[channel_index] * g_GlobalVolume + 128u ) / 256u;
  if ( vol == 0 )
    return;

  if ( !g_flag_stereo )
  {
    int32_t* block = s_mix_scratch + mix_pos;
    if ( g_ChannelSampleBits[channel_index] == 8 )
    {
      const uint8_t* base = (const uint8_t*)g_ChannelSampleAddress[channel_index];
      s1                  = (int8_t)base[pos1];
      s2                  = (int8_t)base[pos2];
      interp              = ( s2 - s1 ) * (int32_t)fraction + ( s1 << 16 );
      *block += (int32_t)( (int64_t)interp * vol / 256 );
    }
    else
    {
      const int16_t* base = (const int16_t*)g_ChannelSampleAddress[channel_index];
      s1                  = base[pos1];
      s2                  = base[pos2];
      interp              = (int32_t)( ( ( (int64_t)( s2 - s1 ) * fraction ) / 256 ) ) + ( s1 << 8 );
      *block += (int32_t)( (int64_t)interp * vol / 256 );
    }
  }
  else
  {
    uint32_t vol_l = ( g_ChannelVolumeLeft[channel_index] * g_GlobalVolume + 128u ) >> 8;
    uint32_t vol_r = ( g_ChannelVolumeRight[channel_index] * g_GlobalVolume + 128u ) >> 8;
    int32_t* block = s_mix_scratch + mix_pos * 2u;
    if ( g_ChannelSampleBits[channel_index] == 8 )
    {
      const uint8_t* base = (const uint8_t*)g_ChannelSampleAddress[channel_index];
      s1                  = (int8_t)base[pos1];
      s2                  = (int8_t)base[pos2];
      interp              = ( s2 - s1 ) * (int32_t)fraction + ( s1 << 16 );
      block[0] += (int32_t)( (int64_t)interp * vol_l / 256 );
      block[1] += (int32_t)( (int64_t)interp * vol_r / 256 );
    }
    else
    {
      const int16_t* base = (const int16_t*)g_ChannelSampleAddress[channel_index];
      s1                  = base[pos1];
      s2                  = base[pos2];
      interp              = (int32_t)( ( ( (int64_t)( s2 - s1 ) * fraction ) / 256 ) ) + ( s1 << 8 );
      block[0] += (int32_t)( (int64_t)interp * vol_l / 256 );
      block[1] += (int32_t)( (int64_t)interp * vol_r / 256 );
    }
  }
}

/* ---- DoMixing (translated from mixer.cpp, 32-bit path only) ---- */

static void do_mixing_32( uint32_t mix_length )
{
  uint32_t channel_index;
  uint32_t counter, counter1, counter2;
  int32_t delta_pos, delta_frac;
  uint32_t pos, frac;
  uint32_t length, loop_begin, loop_end;
  uint32_t end_pos;
  uint32_t mix_length_rest, mix_pos;

  if ( !g_flag_interpolate )
  {
    /* ---- non-interpolated path ---- */
    for ( channel_index = 0; channel_index < g_ChannelLast; channel_index++ )
    {
      if ( g_ChannelActiv[channel_index] != 1 )
        continue;

      delta_pos  = g_DeltaSamplePosition[channel_index];
      delta_frac = g_DeltaSampleFraction[channel_index];
      pos        = g_ChannelSamplePosition[channel_index];
      frac       = g_ChannelSampleFraction[channel_index];

      switch ( g_ChannelLoopMode[channel_index] )
      {
      case 0:
      { /* no loop */
        uint32_t len_pos, len_frac;
        length   = g_ChannelSampleLength[channel_index];
        len_pos  = length - pos;
        len_frac = ( 65536u - frac ) & 65535u;
        if ( len_frac )
          len_pos--;
        s3m_divide_64bit( len_pos, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
        counter = g_division;
        if ( g_div_fraction )
          counter++;
        if ( counter == 0 )
          break;
        if ( counter > mix_length )
          counter = mix_length;
        mix32_channel( channel_index, counter, g_MixerAddress );
        break;
      }
      case 1:
      { /* loop forward */
        uint32_t len_pos, len_frac;
        loop_begin      = g_ChannelLoopBegin[channel_index];
        loop_end        = g_ChannelLoopEnd[channel_index];
        mix_length_rest = mix_length;
        mix_pos         = g_MixerAddress;

        while ( mix_length_rest != 0 )
        {
          len_pos  = loop_end - pos;
          len_frac = ( 65536u - frac ) & 65535u;
          if ( len_frac )
            len_pos--;
          s3m_divide_64bit( len_pos, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
          counter = g_division;
          if ( g_div_fraction )
            counter++;
          if ( counter == 0 )
          {
            mix_length_rest = 0;
            break;
          }
          if ( counter > mix_length_rest )
          {
            counter = mix_length_rest;
            mix32_channel( channel_index, counter, mix_pos );
            mix_pos += counter;
            mix_length_rest = 0;
          }
          else
          {
            mix32_channel( channel_index, counter, mix_pos );
            mix_pos += counter;
            mix_length_rest -= counter;
          }
          frac = (uint32_t)( (int32_t)frac + delta_frac * (int32_t)counter );
          pos  = (uint32_t)( (int32_t)pos + delta_pos * (int32_t)counter ) + ( frac >> 16 );
          frac &= 65535u;
          if ( pos >= loop_end )
            pos = pos - loop_end + loop_begin;
          g_ChannelSamplePosition[channel_index] = pos;
          g_ChannelSampleFraction[channel_index] = frac;
        }
        break;
      }
      default:
        break;
      }
    }
  }
  else
  {
    /* ---- interpolated path ---- */
    for ( channel_index = 0; channel_index < g_ChannelLast; channel_index++ )
    {
      if ( g_ChannelActiv[channel_index] != 1 )
        continue;

      delta_pos  = g_DeltaSamplePosition[channel_index];
      delta_frac = g_DeltaSampleFraction[channel_index];
      pos        = g_ChannelSamplePosition[channel_index];
      frac       = g_ChannelSampleFraction[channel_index];

      switch ( g_ChannelLoopMode[channel_index] )
      {
      case 0:
      { /* no loop */
        uint32_t len_pos, len_frac;
        length          = g_ChannelSampleLength[channel_index];
        mix_length_rest = mix_length;
        mix_pos         = g_MixerAddress;

        len_pos  = length - pos;
        len_frac = ( 65536u - frac ) & 65535u;
        if ( len_frac )
          len_pos--;

        s3m_divide_64bit( len_pos, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
        counter = g_division;
        if ( g_div_fraction )
          counter++;

        if ( len_pos >= 1 )
        {
          s3m_divide_64bit( len_pos - 1u, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
          counter1 = g_division;
          if ( g_div_fraction )
            counter1++;
        }
        else
        {
          counter1 = 0;
        }
        counter2 = counter - counter1;

        if ( counter1 != 0 )
        {
          uint32_t samples_to_mix = ( counter1 > mix_length_rest ) ? mix_length_rest : counter1;
          mix32_ichannel( channel_index, samples_to_mix, mix_pos );
          mix_pos += samples_to_mix;
          mix_length_rest -= samples_to_mix;
          frac = (uint32_t)( (int32_t)frac + delta_frac * (int32_t)samples_to_mix );
          pos  = (uint32_t)( (int32_t)pos + delta_pos * (int32_t)samples_to_mix ) + ( frac >> 16 );
          frac &= 65535u;
          g_ChannelSamplePosition[channel_index] = pos;
          g_ChannelSampleFraction[channel_index] = frac;
        }

        if ( mix_length_rest == 0 )
          break;

        if ( counter2 != 0 )
        {
          uint32_t samples_to_mix = ( counter2 > mix_length_rest ) ? mix_length_rest : counter2;
          uint32_t sample_index;
          for ( sample_index = 0; sample_index < samples_to_mix; sample_index++ )
          {
            end_pos = pos + 1u;
            if ( end_pos >= length )
              end_pos = length - 1u;
            mix32_icarry( channel_index, pos, end_pos, frac, mix_pos );
            frac = (uint32_t)( (int32_t)frac + delta_frac );
            pos  = (uint32_t)( (int32_t)pos + delta_pos ) + ( frac >> 16 );
            frac &= 65535u;
            mix_pos++;
          }
          mix_length_rest -= samples_to_mix;
        }
        break;
      }
      case 1:
      { /* loop forward */
        uint32_t len_pos, len_frac;
        loop_begin      = g_ChannelLoopBegin[channel_index];
        loop_end        = g_ChannelLoopEnd[channel_index];
        mix_length_rest = mix_length;
        mix_pos         = g_MixerAddress;

        while ( mix_length_rest > 0 )
        {
          len_pos  = loop_end - pos;
          len_frac = ( 65536u - frac ) & 65535u;
          if ( len_frac > 0 )
            len_pos--;

          s3m_divide_64bit( len_pos, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
          counter = g_division;
          if ( g_div_fraction )
            counter++;

          if ( len_pos >= 1 )
          {
            s3m_divide_64bit( len_pos - 1u, len_frac, (uint32_t)delta_pos, (uint32_t)delta_frac );
            counter1 = g_division;
            if ( g_div_fraction )
              counter1++;
          }
          else
          {
            counter1 = 0;
          }
          counter2 = counter - counter1;

          if ( counter1 != 0 )
          {
            uint32_t samples_to_mix = ( counter1 > mix_length_rest ) ? mix_length_rest : counter1;
            mix_length_rest -= samples_to_mix;
            mix32_ichannel( channel_index, samples_to_mix, mix_pos );
            mix_pos += samples_to_mix;
            frac = (uint32_t)( (int32_t)frac + delta_frac * (int32_t)samples_to_mix );
            pos  = (uint32_t)( (int32_t)pos + delta_pos * (int32_t)samples_to_mix ) + ( frac >> 16 );
            frac &= 65535u;
            if ( pos >= loop_end )
              pos = pos - loop_end + loop_begin;
            g_ChannelSamplePosition[channel_index] = pos;
            g_ChannelSampleFraction[channel_index] = frac;
          }

          if ( mix_length_rest == 0 )
            break;

          if ( counter2 != 0 )
          {
            uint32_t samples_to_mix = ( counter2 > mix_length_rest ) ? mix_length_rest : counter2;
            uint32_t sample_index;
            for ( sample_index = 0; sample_index < samples_to_mix; sample_index++ )
            {
              end_pos = pos + 1u;
              if ( end_pos >= loop_end )
                end_pos = end_pos - loop_end + loop_begin;
              mix32_icarry( channel_index, pos, end_pos, frac, mix_pos );
              frac = (uint32_t)( (int32_t)frac + delta_frac );
              pos  = (uint32_t)( (int32_t)pos + delta_pos ) + ( frac >> 16 );
              frac &= 65535u;
              mix_pos++;
            }
            mix_length_rest -= samples_to_mix;
            if ( pos >= loop_end )
              pos = pos - loop_end + loop_begin;
          }
          g_ChannelSamplePosition[channel_index] = pos;
          g_ChannelSampleFraction[channel_index] = frac;
        }
        break;
      }
      default:
        break;
      }
    }
  }
}

/* ---- DoPreMixing ---- */

static void prepare_mixing( void )
{
  uint32_t channel_index;
  uint32_t scaling;
  int32_t pan;

  for ( channel_index = 0; channel_index < g_ChannelLast; channel_index++ )
  {
    scaling = calc_scaling( channel_index );
    if ( scaling < 16 )
      g_ChannelActiv[channel_index] = 0;
    g_DeltaSamplePosition[channel_index] = (int32_t)( scaling >> 16 );
    g_DeltaSampleFraction[channel_index] = (int32_t)( scaling & 65535u );

    if ( g_ChannelVolume[channel_index] > 256u )
      g_ChannelVolume[channel_index] = 256u;

    if ( g_flag_stereo )
    {
      pan = (int32_t)( ( (int32_t)( g_ChannelPanning[channel_index] & 511u ) - 128 ) * (int32_t)g_ChannelSeperation + 128 ) / 256;

      g_ChannelVolumeLeft[channel_index] =
        ( pan > 0 ) ? ( g_ChannelVolume[channel_index] * (uint32_t)( 128 - pan ) ) >> 7 : g_ChannelVolume[channel_index];
      g_ChannelVolumeRight[channel_index] =
        ( pan < 0 ) ? ( g_ChannelVolume[channel_index] * (uint32_t)( 128 + pan ) ) >> 7 : g_ChannelVolume[channel_index];
    }
  }
}

void mixer_do_pre_mixing( uint32_t mix_length )
{
  uint32_t channel_index;
  uint32_t pos, frac;
  int32_t dpos, dfrac;
  uint32_t end_pos, end_frac;

  prepare_mixing();

  for ( channel_index = 0; channel_index < g_ChannelLast; channel_index++ )
  {
    pos  = g_ChannelSamplePosition[channel_index];
    frac = g_ChannelSampleFraction[channel_index];

    if ( pos >= g_ChannelLoopEnd[channel_index] )
      g_ChannelLoopMode[channel_index] = 0;
    if ( pos >= g_ChannelSampleLength[channel_index] )
    {
      pos = g_ChannelSamplePosition[channel_index] = g_NextSamplePosition[channel_index] = 0;
      frac = g_ChannelSampleFraction[channel_index] = g_NextSampleFraction[channel_index] = 0;
      g_ChannelActiv[channel_index] = g_NextChannelActiv[channel_index] = 0;
    }

    if ( g_ChannelActiv[channel_index] == 1 )
    {
      dpos  = g_DeltaSamplePosition[channel_index];
      dfrac = g_DeltaSampleFraction[channel_index];

      switch ( g_ChannelLoopMode[channel_index] )
      {
      case 0:
      {
        end_frac = frac + (uint32_t)( (int32_t)dfrac * (int32_t)mix_length );
        end_pos  = pos + (uint32_t)( (int32_t)dpos * (int32_t)mix_length ) + ( end_frac >> 16 );
        end_frac &= 65535u;
        g_NextChannelActiv[channel_index]   = ( end_pos < g_ChannelSampleLength[channel_index] ) ? 1u : 0u;
        g_NextSamplePosition[channel_index] = g_NextChannelActiv[channel_index] ? end_pos : 0u;
        g_NextSampleFraction[channel_index] = g_NextChannelActiv[channel_index] ? end_frac : 0u;
        break;
      }
      case 1:
      {
        end_frac = frac + (uint32_t)( (int32_t)dfrac * (int32_t)mix_length );
        end_pos  = pos + (uint32_t)( (int32_t)dpos * (int32_t)mix_length ) + ( end_frac >> 16 );
        end_frac &= 65535u;
        while ( end_pos >= g_ChannelLoopEnd[channel_index] )
          end_pos = end_pos - g_ChannelLoopEnd[channel_index] + g_ChannelLoopBegin[channel_index];
        g_NextSamplePosition[channel_index] = end_pos;
        g_NextSampleFraction[channel_index] = end_frac;
        g_NextChannelActiv[channel_index]   = 1u;
        break;
      }
      default:
        break;
      }
    }
  }

  do_mixing_32( mix_length );

  for ( channel_index = 0; channel_index < g_ChannelLast; channel_index++ )
  {
    if ( g_ChannelActiv[channel_index] == 1 )
    {
      g_ChannelSamplePosition[channel_index] = g_NextSamplePosition[channel_index];
      g_ChannelSampleFraction[channel_index] = g_NextSampleFraction[channel_index];
      g_ChannelActiv[channel_index]          = g_NextChannelActiv[channel_index];
    }
  }
}

/* ---- Master volume table (calcMasterVolume32 port) ---- */

void mixer_calc_master_vol32( uint32_t master_volume, int32_t* table_centre )
{
  int32_t counter;
  double value;
  int32_t val;

  if ( master_volume < 128u )
    master_volume = 128u;

  for ( counter = -8192; counter < 8191; counter++ )
    table_centre[counter] = (int32_t)master_volume;

  if ( master_volume == 32768u )
    return;

  value = 32768.0 / (double)master_volume * 32.0 * 0.80;
  val   = (int32_t)value;

  if ( val < 1 )
    return;

  {
    /*
         * Original quirk: `test` is only (re)assigned while counter < 2*val.
         * For counter >= 2*val it retains its last value (~2/3 * MasterVolume),
         * NOT 128 — do NOT add an else branch here (see bugs.md P-3 / O-2).
         * Loud modules (e.g. 669, MasterVolume 12288) reach these high indices;
         * quiet S3M modules don't, which masks the difference.
         */
    int32_t test = 128;
    for ( counter = val; counter < 8192; counter++ )
    {
      if ( counter < 2 * val )
        test = (int32_t)( (double)( 4 * val - counter ) * ( (double)master_volume / (double)( 3 * val ) ) );
      if ( test < 128 )
        test = 128;
      table_centre[counter]     = test;
      table_centre[counter - 1] = test; /* original writes counter and counter-1 */
    }
  }
}

/* ---- Output conversion (MasterVol16for32ss port) ---- */

void mixer_convert_to_s16( int16_t* out, uint32_t frame_count, const int32_t* master_vol_table )
{
  uint32_t sample_count = frame_count * ( g_flag_stereo ? 2u : 1u );
  uint32_t i;
  for ( i = 0; i < sample_count; i++ )
  {
    int32_t mix_sample = s_mix_scratch[i];
    int32_t idx        = mix_sample >> 18; /* signed arithmetic shift */
    int64_t scaled     = (int64_t)mix_sample * master_vol_table[idx];
    int32_t result     = (int32_t)( scaled / 0x800000L ) + 0x8000;
    if ( result & (int32_t)0xFFFF0000 )
    {
      result = ( result < 0 ) ? 0 : 0xFFFF;
    }
    out[i] = (int16_t)( result - 32768 );
  }
}
