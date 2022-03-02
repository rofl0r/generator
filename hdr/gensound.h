#ifndef GENSOUND_HEADER_FILE
#define GENSOUND_HEADER_FILE

#ifndef JFM
#  include "ym2612/support.h"
#  include "ym2612/fm.h"
#endif

#define SOUND_MAXRATE 48000
#define SOUND_SAMPLERATE 44100

extern int sound_debug;
extern int sound_feedback;
extern unsigned int sound_minfields;
extern unsigned int sound_maxfields;
extern unsigned int sound_speed;
extern unsigned int sound_sampsperfield;
extern unsigned int sound_threshold;
extern uint8 sound_regs1[256];
extern uint8 sound_regs2[256];
extern uint8 sound_address1;
extern uint8 sound_address2;
extern uint8 sound_keys[8];
extern unsigned int sound_on;
extern unsigned int sound_psg;
extern unsigned int sound_fm;
extern uint16 sound_soundbuf[2][SOUND_MAXRATE / 50];
extern unsigned int sound_filter;

int sound_start(void);
void sound_stop(void);
int sound_init(void);
void sound_final(void);
int sound_reset(void);
void sound_startfield(void);
void sound_endfield(void);
void sound_genreset(void);
uint8 sound_ym2612fetch(uint8 addr);
void sound_ym2612store(uint8 addr, uint8 data);
void sound_sn76496store(uint8 data);
void sound_genreset(void);
void sound_line(void);

#endif /* GENSOUND_HEADER_FILE */
