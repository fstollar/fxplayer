#ifndef FX_MIXER_SCALAR_H
#define FX_MIXER_SCALAR_H

#include <stdint.h>
#include <stddef.h>

#define FX_MAXCHANNELS    256
#define FX_INTERNAL_BLOCK 4096

/* Global mixer config (set by fx_load from fx_config) */
extern uint32_t g_MixSpeed;
extern uint32_t g_MasterVolume;        /* format-specific volume scale; NOT a 0-64 UI value */
extern uint32_t g_flag_stereo;         /* 0=mono, 1=stereo */
extern uint32_t g_flag_interpolate;
extern uint32_t g_flag_soft_clip;
extern uint32_t g_ChannelSeparation;   /* stereo separation width: 0=centre, 256=full L/R split */
extern uint32_t g_GlobalVolume;        /* in-song global volume (S3M Vxx effect), 0-64 */

/* Mixer address and state */
extern uint32_t g_MixerAddress;        /* current write offset into the 32-bit mix accumulator */
extern uint32_t g_ChannelUsed, g_ChannelLast;

/* Per-channel arrays [FX_MAXCHANNELS] */
extern uint32_t  g_ChannelActive[FX_MAXCHANNELS];
extern uint32_t  g_ChannelFlag[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSampleNr[FX_MAXCHANNELS];
extern uint32_t  g_ChannelVolume[FX_MAXCHANNELS];
extern uint32_t  g_ChannelPanning[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSampleBits[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSampleMode[FX_MAXCHANNELS];
extern uintptr_t g_ChannelSampleAddress[FX_MAXCHANNELS]; /* raw byte pointer */
extern uint32_t  g_ChannelSampleLength[FX_MAXCHANNELS];
extern uint32_t  g_ChannelLoopMode[FX_MAXCHANNELS];
extern uint32_t  g_ChannelLoopBegin[FX_MAXCHANNELS];
extern uint32_t  g_ChannelLoopEnd[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSamplePosition[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSampleFraction[FX_MAXCHANNELS];
extern uint32_t  g_ChannelSampleFrequency[FX_MAXCHANNELS];
extern int32_t   g_ChannelLastValueLeft[FX_MAXCHANNELS];
extern int32_t   g_ChannelLastValueRight[FX_MAXCHANNELS];
extern uint32_t  g_ChannelVolumeLeft[FX_MAXCHANNELS];
extern uint32_t  g_ChannelVolumeRight[FX_MAXCHANNELS];
extern int32_t   g_DeltaSamplePosition[FX_MAXCHANNELS];
extern int32_t   g_DeltaSampleFraction[FX_MAXCHANNELS];
extern uint32_t  g_NextSamplePosition[FX_MAXCHANNELS];
extern uint32_t  g_NextSampleFraction[FX_MAXCHANNELS];
extern uint32_t  g_NextChannelActive[FX_MAXCHANNELS];

/*
 * Workspace-allocated master volume table.
 * Set by each format loader (mod_load, s3m_load, m669_load) to point into
 * its own workspace, then consumed by mixer_convert_to_s16.
 */
extern int32_t *g_master_vol_table;

/* API called by format loaders and engine/fx.c */

/* Zero all mixer state. Called by fx_load before loading a new module. */
void mixer_reset(void);

/* Zero only the 32-bit mix accumulator buffers, leaving per-channel config intact.
   Called at the start of each render block before accumulating channels. */
void mixer_clear(void);

/* Accumulate all active channels into the 32-bit mix buffer for mix_length frames. */
void mixer_do_pre_mixing(uint32_t mix_length);

/* Apply master_vol_table lookup and clip/convert the 32-bit accumulator to interleaved s16 PCM. */
void mixer_convert_to_s16(int16_t *out, uint32_t frame_count,
                          const int32_t *master_vol_table);

/*
 * Build the 16385-entry soft-clip volume lookup table at table_centre.
 * The table maps 32-bit accumulator values to s16 output samples, scaled by
 * master_volume. Called by format loaders at load time and by fx_set_volume
 * when the user changes the volume.
 */
void mixer_calc_master_vol32(uint32_t master_volume, int32_t *table_centre);

#endif
