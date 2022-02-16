#ifndef JFM
#  include "support.h"
#  include "fm.h"
#endif

#define SOUND_SAMPLERATE 22050
#define SOUND_DEVICE "/dev/dsp"

/* #include "SN76489.h" */

/* extern SN76489 psg_cpu; */
extern int sound_feedback;
extern int sound_debug;
extern int sound_minfields;
extern int sound_maxfields;

int sound_start(void);
void sound_stop(void);
int sound_init(void);
void sound_final(void);
int sound_reset(void);
void sound_genreset(void);
void sound_process(void);
void sound_endfield(void);

#define sound_psgwrite(data) WriteSN76489(&psg_cpu, data)
#define WriteSN76489(x,y) 
