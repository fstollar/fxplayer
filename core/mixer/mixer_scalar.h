#ifndef FX_MIXER_SCALAR_H
#define FX_MIXER_SCALAR_H

#include <stdint.h>
#include <stddef.h>

#define FX_MAXCHANNELS    256
#define FX_INTERNAL_BLOCK 4096

/* Global mixer config (set by fx_load from fx_config) */
extern uint32_t g_MixSpeed;
extern uint32_t g_MasterVolume;
extern uint32_t g_flag_stereo;      /* 0=mono, 1=stereo */
extern uint32_t g_flag_interpolate;
extern uint32_t g_flag_soft_clip;
extern uint32_t g_ChannelSeperation;
extern uint32_t g_GlobalVolume;

/* Mixer address and state */
extern uint32_t g_MixerAddress;
extern uint32_t g_ChannelUsed, g_ChannelLast;

/* Per-channel arrays [FX_MAXCHANNELS] */
extern uint32_t g_ChannelActiv[FX_MAXCHANNELS];
extern uint32_t g_ChannelFlag[FX_MAXCHANNELS];
extern uint32_t g_ChannelSampleNr[FX_MAXCHANNELS];
extern uint32_t g_ChannelVolume[FX_MAXCHANNELS];
extern uint32_t g_ChannelPanning[FX_MAXCHANNELS];
extern uint32_t g_ChannelSampleBits[FX_MAXCHANNELS];
extern uint32_t g_ChannelSampleMode[FX_MAXCHANNELS];
extern uintptr_t g_ChannelSampleAddress[FX_MAXCHANNELS]; /* raw byte pointer */
extern uint32_t g_ChannelSampleLength[FX_MAXCHANNELS];
extern uint32_t g_ChannelLoopMode[FX_MAXCHANNELS];
extern uint32_t g_ChannelLoopBegin[FX_MAXCHANNELS];
extern uint32_t g_ChannelLoopEnd[FX_MAXCHANNELS];
extern uint32_t g_ChannelSamplePosition[FX_MAXCHANNELS];
extern uint32_t g_ChannelSampleFraction[FX_MAXCHANNELS];
extern uint32_t g_ChannelSampleFrequence[FX_MAXCHANNELS];
extern int32_t  g_ChannelLastValueLeft[FX_MAXCHANNELS];
extern int32_t  g_ChannelLastValueRight[FX_MAXCHANNELS];
extern uint32_t g_ChannelVolumeLeft[FX_MAXCHANNELS];
extern uint32_t g_ChannelVolumeRight[FX_MAXCHANNELS];
extern int32_t  g_DeltaSamplePosition[FX_MAXCHANNELS];
extern int32_t  g_DeltaSampleFraction[FX_MAXCHANNELS];
extern uint32_t g_NextSamplePosition[FX_MAXCHANNELS];
extern uint32_t g_NextSampleFraction[FX_MAXCHANNELS];
extern uint32_t g_NextChannelActiv[FX_MAXCHANNELS];

/* API called by s3m.c and engine/fx.c */
void mixer_reset(void);
void mixer_clear(void);
void mixer_do_pre_mixing(uint32_t mix_length);
void mixer_convert_to_s16(int16_t *out, uint32_t frame_count,
                          const int32_t *master_vol_table);
void mixer_calc_master_vol32(uint32_t master_volume, int32_t *table_centre);

#endif
