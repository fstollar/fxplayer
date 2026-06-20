#ifndef FX_FORMAT_MOD_H
#define FX_FORMAT_MOD_H

#include <stdint.h>
#include <stddef.h>

#define MOD_MAXCHANNELS 8
#define MOD_MAXSAMPLES  31
#define MOD_MAXPATTERNS 128

/* Period table [16 finetunes][84 notes] — filled at load time */
extern uint32_t MOD_Periodes[16][84];

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
extern uint8_t  MOD_ChannelActiv[MOD_MAXCHANNELS];
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

extern uint32_t MOD_Periode[MOD_MAXCHANNELS];
extern int32_t  MOD_PeriodeAdjust[MOD_MAXCHANNELS];
extern uint32_t MOD_Frequence[MOD_MAXCHANNELS];

/* API */
size_t  mod_workspace_bytes(const uint8_t *data, size_t size);
int     mod_load(const uint8_t *data, size_t size,
                 uint8_t *ws, size_t ws_size);
void    mod_close(void);
void    mod_render_block(uint32_t block_frames);
uint8_t mod_is_done(void);

/* Called by effect module */
void MOD_initvariables(void);
void MOD_unpack_row(uint32_t pat_nr, uint32_t row_nr);
void MOD_GetNewSample(uint32_t channel);
void MOD_GetNewNote(uint32_t channel);
void MOD_GetNewEffect(void);
void MOD_read_row(void);
void MOD_goRowOrder(void);

uint32_t Calc_AMIGAperiode(uint32_t note, uint32_t finetune);

#endif /* FX_FORMAT_MOD_H */
