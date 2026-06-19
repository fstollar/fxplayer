#ifndef FX_FORMAT_S3M_H
#define FX_FORMAT_S3M_H

#include <stdint.h>
#include <stddef.h>

#define S3M_MAXCHANNELS 32

/* Note period table C..B */
extern uint32_t S3M_NotePeriodes[12];

/* Song state */
extern uint32_t S3M_OrderNum, S3M_InstrumentNum, S3M_PatternNum;
extern uint32_t S3M_channels;
extern uint8_t  S3M_RowBuffer[S3M_MAXCHANNELS * 5];

extern uint32_t S3M_row, S3M_jump, S3M_nextrow, S3M_nextorder;
extern uint32_t S3M_tick, S3M_Order, S3M_Pattern;
extern uint32_t S3M_TickLength;
extern uint32_t S3M_buffer_rest, S3M_tick_rest;

extern uint8_t  S3M_flag_stereo;
extern uint8_t  S3M_flag_amigalimits;
extern uint8_t  S3M_flag_st3volslides;
extern uint8_t  S3M_flag_note;

extern uint8_t  S3M_tempo, S3M_speed, S3M_GlobalVolume;

extern uint8_t  S3M_ChannelActiv[S3M_MAXCHANNELS];
extern uint8_t  S3M_SampleNr[S3M_MAXCHANNELS];
/* S3M_SampleAddress holds a raw byte pointer stored as uintptr_t */
extern uintptr_t S3M_SampleAddress[S3M_MAXCHANNELS];
extern uint32_t S3M_SampleLength[S3M_MAXCHANNELS];
extern uint32_t S3M_SamplePosition[S3M_MAXCHANNELS];
extern uint32_t S3M_SampleFraction[S3M_MAXCHANNELS];
extern uint32_t S3M_SampleLoopBegin[S3M_MAXCHANNELS];
extern uint32_t S3M_SampleLoopEnd[S3M_MAXCHANNELS];
extern uint32_t S3M_SampleC4SPD[S3M_MAXCHANNELS];
extern uint8_t  S3M_SampleFlags[S3M_MAXCHANNELS];

extern uint8_t  S3M_Volume[S3M_MAXCHANNELS];
extern uint8_t  S3M_Panning[S3M_MAXCHANNELS];
extern uint8_t  S3M_Note[S3M_MAXCHANNELS];
extern uint8_t  S3M_Octave[S3M_MAXCHANNELS];
extern uint8_t  S3M_Effect[S3M_MAXCHANNELS];
extern uint8_t  S3M_EffectInfo[S3M_MAXCHANNELS];
extern uint8_t  S3M_LastEffect[S3M_MAXCHANNELS];
extern uint8_t  S3M_LastEffectInfo[S3M_MAXCHANNELS];

extern uint32_t S3M_Periode[S3M_MAXCHANNELS];
extern int32_t  S3M_PeriodeAdjust[S3M_MAXCHANNELS];
extern uint32_t S3M_Frequence[S3M_MAXCHANNELS];

/* Pointer to MasterVolumeTable (workspace-carved, centre at [0]) */
extern int32_t *g_master_vol_table;

/* API */
size_t   s3m_workspace_bytes(const uint8_t *data, size_t size);
int      s3m_load(const uint8_t *data, size_t size,
                  uint8_t *ws, size_t ws_size);
void     s3m_close(void);
void     s3m_render_block(uint32_t block_frames);
uint8_t  s3m_is_done(void);

/* Called by effect module */
void S3M_GetNewSample(uint32_t channel);
void S3M_GetNewNote(uint32_t channel);
void S3M_GetNewEffect(void);
void S3M_read_row(void);
void S3M_goRowOrder(void);
void S3M_initvariables(void);
void S3M_unpack_row(uint32_t pat_nr, uint32_t row_nr);

uint32_t Calc_st3periode(uint32_t note, uint32_t octave, uint32_t c4spd);

#endif
