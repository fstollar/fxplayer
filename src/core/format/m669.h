#ifndef FX_FORMAT_M669_H
#define FX_FORMAT_M669_H

#include <stdint.h>
#include <stddef.h>

#define M669_CHANNELS   8
#define M669_MAXSAMPLES 64
#define M669_MAXPATTERNS 128

/* Period table [16 finetunes][72 notes] — filled at load time */
extern uint32_t M669_Periodes[16][72];

/* Song meta */
extern uint32_t M669_samples;
extern uint32_t M669_patterns;
extern uint8_t  M669_OrderNum;

/* Runtime state */
extern uint8_t  M669_RowBuffer[M669_CHANNELS * 5];

extern uint32_t M669_LastPattern, M669_LastRow;
extern uint32_t M669_TickLength;
extern uint32_t M669_row, M669_jump, M669_nextrow, M669_nextorder;
extern uint32_t M669_tick, M669_Order, M669_Pattern;
extern uint32_t M669_buffer_rest, M669_tick_rest;

extern uint8_t  M669_tempo, M669_speed;

/* Per-channel arrays */
extern uint8_t  M669_ChannelActiv[M669_CHANNELS];
extern uint8_t  M669_SampleNr[M669_CHANNELS];
extern uintptr_t M669_SampleAddress[M669_CHANNELS];
extern uint32_t M669_SampleLength[M669_CHANNELS];
extern uint32_t M669_SamplePosition[M669_CHANNELS];
extern uint32_t M669_SampleFraction[M669_CHANNELS];
extern uint8_t  M669_SampleLoop[M669_CHANNELS];
extern uint32_t M669_SampleLoopBegin[M669_CHANNELS];
extern uint32_t M669_SampleLoopEnd[M669_CHANNELS];
extern uint32_t M669_SampleFinetune[M669_CHANNELS];

extern uint8_t  M669_Volume[M669_CHANNELS];
extern uint8_t  M669_Panning[M669_CHANNELS];
extern uint8_t  M669_Note[M669_CHANNELS];
extern uint8_t  M669_Effect[M669_CHANNELS];
extern uint8_t  M669_EffectInfo[M669_CHANNELS];
extern uint8_t  M669_LastEffect[M669_CHANNELS];
extern uint8_t  M669_LastEffectInfo[M669_CHANNELS];

extern uint32_t M669_Periode[M669_CHANNELS];
extern int32_t  M669_PeriodeAdjust[M669_CHANNELS];
extern uint32_t M669_Frequence[M669_CHANNELS];

/* Orderlist and per-pattern speed/break lists (pointers into workspace) */
extern uint8_t *M669_Orderlist;
extern uint8_t *M669_Speedlist;
extern uint8_t *M669_Breaklist;

/* API */

/* Parse the 669 header and return the required workspace buffer size. Returns 0 on error. */
size_t  m669_workspace_bytes(const uint8_t *data, size_t size);

/* Load a 669 module into caller-supplied workspace. Returns 0=ok, -1=format error, -2=workspace too small. */
int     m669_load(const uint8_t *data, size_t size,
                  uint8_t *ws, size_t ws_size);

void    m669_close(void);

/* Mix block_frames frames into the global mixer accumulator and advance the sequencer. */
void    m669_render_block(uint32_t block_frames);

/* Returns non-zero when the song has played through and looped back to the start. */
uint8_t m669_is_done(void);

/* Called by effect module */

/* Reset all playback state variables to start-of-song defaults. */
void M669_initvariables(void);

/* Decode a 5-byte-per-channel 669 pattern row into M669_RowBuffer. */
void M669_unpack_row(uint32_t pat_nr, uint32_t row_nr);

/* Reload sample pointer, length, and loop bounds for the channel from the instrument list. */
void M669_GetNewSample(uint32_t channel);

/* Compute the playback period/frequency for the current note on the given channel. */
void M669_GetNewNote(uint32_t channel);

/* Store and prepare effect parameters from the current row for all active channels. */
void M669_GetNewEffect(void);

/* Decode one row from the current pattern and dispatch notes/effects to all channels. */
void M669_read_row(void);

/* Advance to the next row, or the next order-list entry when the pattern ends. */
void M669_goRowOrder(void);

/* Convert 669 note index + finetune to the mixer period value. */
uint32_t Calc_669periode(uint32_t note, uint32_t finetune);

#endif /* FX_FORMAT_M669_H */
