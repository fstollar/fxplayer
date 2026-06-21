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

/* API */

/* Parse the S3M header and return the required workspace buffer size. Returns 0 on error. */
size_t   s3m_workspace_bytes(const uint8_t *data, size_t size);

/* Load an S3M module into caller-supplied workspace. Returns 0=ok, -1=format error, -2=workspace too small. */
int      s3m_load(const uint8_t *data, size_t size,
                  uint8_t *ws, size_t ws_size);

void     s3m_close(void);

/* Mix block_frames frames into the global mixer accumulator and advance the sequencer. */
void     s3m_render_block(uint32_t block_frames);

/* Returns non-zero when the song has played through and looped back to the start. */
uint8_t  s3m_is_done(void);

/* Called by effect module */

/* Reload sample pointer, length, and loop bounds for the channel from the instrument list. */
void S3M_GetNewSample(uint32_t channel);

/* Compute the playback period/frequency for the current note on the given channel. */
void S3M_GetNewNote(uint32_t channel);

/* Store and prepare effect parameters from the current row for all active channels. */
void S3M_GetNewEffect(void);

/* Decode one row from the current pattern and dispatch notes/effects to all channels. */
void S3M_read_row(void);

/* Advance to the next row, or the next order-list entry when the pattern ends. Handles jump effects. */
void S3M_goRowOrder(void);

/* Reset all playback state variables to start-of-song defaults. */
void S3M_initvariables(void);

/* Decode compressed S3M pattern data for the given pattern/row into S3M_RowBuffer. */
void S3M_unpack_row(uint32_t pat_nr, uint32_t row_nr);

/* Convert ST3 note+octave+c4spd (C4 sample rate in Hz) to the mixer period value. */
uint32_t Calc_st3periode(uint32_t note, uint32_t octave, uint32_t c4spd);

#endif
