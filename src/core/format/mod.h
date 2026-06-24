#ifndef FX_FORMAT_MOD_H
#define FX_FORMAT_MOD_H

#include <stdint.h>
#include <stddef.h>

#define MOD_MAXCHANNELS 8
#define MOD_MAXSAMPLES  31
#define MOD_MAXPATTERNS 128

/* Period table [16 finetunes][84 notes] — filled at load time */
extern uint32_t MOD_Periods[16][84];

/* Song meta */
extern uint32_t MOD_channels;
extern uint32_t MOD_samples;
extern uint32_t MOD_patterns;
extern uint8_t  MOD_OrderNum;

/* Runtime state */
extern uint8_t  MOD_RowBuffer[MOD_MAXCHANNELS * 4];

extern uint32_t MOD_LastPattern, MOD_LastRow;
extern uint32_t MOD_TickLength;
extern uint32_t MOD_row, MOD_jump, MOD_nextrow, MOD_nextorder;
extern uint32_t MOD_tick, MOD_Order, MOD_Pattern;
extern uint32_t MOD_buffer_rest, MOD_tick_rest;

extern uint8_t  MOD_flag_amigalimits;
extern uint8_t  MOD_flag_note;
extern uint8_t  MOD_tempo, MOD_speed;

/* Per-channel arrays */
extern uint8_t  MOD_ChannelActive[MOD_MAXCHANNELS];
extern uint8_t  MOD_SampleNr[MOD_MAXCHANNELS];
extern uintptr_t MOD_SampleAddress[MOD_MAXCHANNELS];
extern uint32_t MOD_SampleLength[MOD_MAXCHANNELS];
extern uint32_t MOD_SamplePosition[MOD_MAXCHANNELS];
extern uint32_t MOD_SampleFraction[MOD_MAXCHANNELS];
extern uint8_t  MOD_SampleLoop[MOD_MAXCHANNELS];
extern uint32_t MOD_SampleLoopBegin[MOD_MAXCHANNELS];
extern uint32_t MOD_SampleLoopEnd[MOD_MAXCHANNELS];
extern uint32_t MOD_SampleFinetune[MOD_MAXCHANNELS];

extern uint8_t  MOD_Volume[MOD_MAXCHANNELS];
extern uint8_t  MOD_Panning[MOD_MAXCHANNELS];
extern uint8_t  MOD_Note[MOD_MAXCHANNELS];
extern uint8_t  MOD_Effect[MOD_MAXCHANNELS];
extern uint8_t  MOD_EffectInfo[MOD_MAXCHANNELS];
extern uint8_t  MOD_LastEffect[MOD_MAXCHANNELS];
extern uint8_t  MOD_LastEffectInfo[MOD_MAXCHANNELS];

extern uint32_t MOD_Period[MOD_MAXCHANNELS];
extern int32_t  MOD_PeriodAdjust[MOD_MAXCHANNELS];
extern uint32_t MOD_Frequency[MOD_MAXCHANNELS];

/* API */

/* Parse the MOD header and return the required workspace buffer size. Returns 0 on error. */
size_t  mod_workspace_bytes(const uint8_t *data, size_t size);

/* Load a MOD module into caller-supplied workspace. Returns 0=ok, -1=format error, -2=workspace too small. */
int     mod_load(const uint8_t *data, size_t size,
                 uint8_t *ws, size_t ws_size);

void    mod_close(void);

/* Mix block_frames frames into the global mixer accumulator and advance the sequencer. */
void    mod_render_block(uint32_t block_frames);

/* Returns non-zero when the song has played through to a natural end (order list exhausted). */
uint8_t     mod_is_done(void);

/* Returns how many times the song has looped (B effect to order 0, or order list exhausted). */
uint32_t    mod_song_loops(void);

/* Returns the null-terminated song title (up to 20 chars). Empty string before load. */
const char *mod_song_title(void);

/* Mark one song loop; called by the effect engine on B effect to order 0. */
void     mod_mark_looped(void);

/* Reset s_dat_ready to 1 so rendering resumes after a natural end. */
void     mod_restart(void);

/* Called by effect module */

/* Reset all playback state variables to start-of-song defaults. */
void MOD_initvariables(void);

/* Decode a 4-byte-per-channel MOD pattern row into MOD_RowBuffer. */
void MOD_unpack_row(uint32_t pat_nr, uint32_t row_nr);

/* Reload sample pointer, length, and loop bounds for the channel from the instrument list. */
void MOD_GetNewSample(uint32_t channel);

/* Compute the playback period/frequency for the current note on the given channel. */
void MOD_GetNewNote(uint32_t channel);

/* Store and prepare effect parameters from the current row for all active channels. */
void MOD_GetNewEffect(void);

/* Decode one row from the current pattern and dispatch notes/effects to all channels. */
void MOD_read_row(void);

/* Advance to the next row, or the next order-list entry when the pattern ends. Handles jump effects. */
void MOD_goRowOrder(void);

/* Convert Amiga MOD note index + finetune to the Amiga hardware period (Paula clock divisor). */
uint32_t Calc_AMIGAperiod(uint32_t note, uint32_t finetune);

#endif /* FX_FORMAT_MOD_H */
