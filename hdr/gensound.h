#ifndef JFM
#  include "support.h"
#  include "fm.h"
#endif

#define SOUND_SAMPLERATE 22050
#define SOUND_DEVICE "/dev/dsp"


/* #include "SN76489.h" */

/* extern SN76489 psg_cpu; */
extern int sound_feedback;

int sound_init(void);
int sound_reset(void);
void sound_genreset(void);
void sound_process(void);
void sound_endfield(void);

#ifdef JFM
  uint8 sound_ym2612fetch(uint8 addr);
  void sound_ym2612store(uint8 addr, uint8 data);
#else
#  define sound_ym2612fetch(addr) YM2612Read(0, addr)
#  define sound_ym2612store(addr, data) YM2612Write(0, addr, data)
#endif

#define sound_psgwrite(data) WriteSN76489(&psg_cpu, data)
#define WriteSN76489(x,y) 

