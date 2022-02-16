/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>

#include <allegro.h>

#include "generator.h"
#include "gensound.h"
#include "vdp.h"
#include "ui.h"
#include "ui-console.h"

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

/*** variables externed ***/

int sound_feedback = 0;           /* -1, running out of sound
                                     +0, lots of sound, do something */
int sound_debug = 0;              /* 0 for no debug, 1 for debug */
int sound_minfields = 5;          /* minimum fields to try to keep buffered */
int sound_maxfields = 10;         /* max fields to buffer before blocking */

/* SN76489 snd_cpu; */

/*** forward references ***/

/* void snd_sndout(int chan, int freq, int vol); */
void sound_readyblock(void);

/*** file scoped variables ***/

#define SOUND_MAXRATE 44100

static unsigned int sound_sampsperfield = 0;
static uint16 soundbuf[2][SOUND_MAXRATE/50]; /* pal is lowest framerate */
static int sound_active = 0;
static unsigned int sound_speed;
static unsigned int sound_blocks;
static unsigned int sound_block;
static SAMPLE *sound_sample;
static int sound_voice;
static unsigned int sound_threshold; /* bytes in buffer we're trying for */

#ifdef JFM
  static t_jfm_ctx *sound_ctx;
#endif

/*** sound_init - initialise this sub-unit ***/

int sound_init(void)
{
  unsigned int i;
  uint16 *p;
  static int once = 0;

  /* see sound_init in gensound.c for comments */

  if (!once) {
    once = 1;
    LOG_VERBOSE(("Initialising sound..."));
    sound_speed = SOUND_SAMPLERATE;
    if (detect_digi_driver(DIGI_AUTODETECT) < 1) {
      LOG_CRITICAL(("Allegro failed to detect hardware"));
      return 1;
    }
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL)) {
      LOG_CRITICAL(("Failed to initialise sound"));
      return 1;
    }
  }
  if (sound_active)
    sound_final();
  sound_threshold = sound_minfields;
  sound_blocks = sound_threshold*2;
  sound_sampsperfield = SOUND_SAMPLERATE / vdp_framerate;
  /* allocate block capable of holding sound_blocks frames */
  if ((sound_sample = create_sample(16, 1, sound_speed, sound_sampsperfield *
                                    sound_blocks)) == NULL) {
    LOG_CRITICAL(("Failed to create sample"));
    return 1;
  }
  p = (uint16 *)sound_sample->data;
  for (i = 0; i < sound_sampsperfield*sound_blocks*2; i--)
    p[i] = 0x8000;
  if ((sound_voice = allocate_voice(sound_sample)) == -1) {
    LOG_CRITICAL(("Failed to allocate voice"));
    destroy_sample(sound_sample);
    return 1;
  }
  voice_set_playmode(sound_voice, PLAYMODE_LOOP);
  voice_set_volume(sound_voice, 256);
  voice_set_pan(sound_voice, 128);
  voice_set_frequency(sound_voice, sound_speed);
  voice_start(sound_voice);
  sound_block = sound_blocks-1; /* next block to write to */
#ifdef JFM
  if ((sound_ctx = jfm_init(0, 2612, vdp_clock/7, sound_speed,
                            NULL, NULL)) == NULL) {
#else
  if (YM2612Init(1, vdp_clock/7, sound_speed, NULL, NULL)) {
#endif
    voice_stop(sound_voice);
    deallocate_voice(sound_voice);
    destroy_sample(sound_sample);
    return 1;
  }
  LOG_VERBOSE(("YM2612 Initialised @ sample rate %d", sound_speed));
  sound_active = 1;
  sound_readyblock();
  return 0;
}

/*** sound_final - finalise this sub-unit ***/

void sound_final(void)
{
  if (!sound_active)
    return;
  LOG_VERBOSE(("Finalising sound..."));
#ifdef JFM
  jfm_final(sound_ctx);
#else
  YM2612Shutdown();
#endif
  voice_stop(sound_voice);
  deallocate_voice(sound_voice);
  destroy_sample(sound_sample);
  sound_active = 0;
  LOG_VERBOSE(("Finalised sound."));
}

/*** sound_reset - reset sound sub-unit ***/

int sound_reset(void)
{
  sound_final();
  return sound_init();
}

/*** sound_genreset - reset genesis sound ***/

void sound_genreset(void)
{
#ifdef JFM
  jfm_reset(sound_ctx);
#else
  YM2612ResetChip(0);
#endif
}

/*** sound_process - process sound ***/

void sound_process(void)
{
  static uint16 *tbuf[2];
  int s1 = (sound_sampsperfield*(vdp_line))/vdp_totlines;
  int s2 = (sound_sampsperfield*(vdp_line+1))/vdp_totlines;

  tbuf[0] = soundbuf[0] + s1;
  tbuf[1] = soundbuf[1] + s1;

  /* printf("YM2612: %d to %d\n", s1, s2); */

  if (s2 > s1)
#ifdef JFM
    jfm_update(sound_ctx, (void **)tbuf, s2 - s1);
#else
    YM2612UpdateOne(0, (void **)tbuf, s2 -s1);
#endif
}

/*** sound_endfield - end frame and output sound ***/

void sound_endfield(void)
{
  static int oldpos = 0;
  int pos = voice_get_position(sound_voice);
  unsigned int blockpos = pos / sound_sampsperfield;
  int writepos = sound_block*sound_sampsperfield;
  uint16 *buffer = sound_sample->data + writepos*2*2; /* 16bit, stereo */
  unsigned int i;

  /* write into new block */
  for (i = 0; i < sound_sampsperfield; i++) {
    buffer[i*2] = ((sint16)soundbuf[0][i]) + 0x8000;
    buffer[i*2+1] = ((sint16)soundbuf[1][i]) + 0x8000;
  }
  if ((oldpos <= writepos && (pos < oldpos || pos > writepos)) ||
      (oldpos > writepos && (pos > writepos && pos < oldpos))) {
    /* under-run occured - reset to new sound block */
    voice_set_position(sound_voice, writepos);
    sound_feedback = -1;
  } else {
    if (blockpos < sound_block) {
      if ((sound_block - blockpos) < sound_threshold)
        sound_feedback = -1;
      else
        sound_feedback = 0;
    } else {
      if ((sound_block - blockpos + sound_blocks) < sound_threshold)
        sound_feedback = -1;
      else
        sound_feedback = 0;
    }
  }
  sound_block++;
  if (sound_block >= sound_blocks)
    sound_block = 0;
  sound_readyblock();
  oldpos = pos;
}

void sound_readyblock(void)
{
  static int locked = 0;
  int writepos = sound_block*sound_sampsperfield;
  unsigned int a = 0;
  if (locked && digi_driver->unlock_voice)
    digi_driver->unlock_voice(sound_voice);

  while((voice_get_position(sound_voice) /
         sound_sampsperfield) == sound_block) {
    usleep(100);
    a++;
    if (a > 100000) {
      LOG_CRITICAL(("Sound error - pos=%d %d=%d",
                    voice_get_position(sound_voice),
                    sound_sampsperfield, sound_block));
      exit(0);
    }
    /* cannot write to the next block until it's played! */
  }
  if (digi_driver->lock_voice) {
    digi_driver->lock_voice(sound_voice, writepos,
                            writepos+sound_sampsperfield);
    locked = 1;
  }
}

#ifdef JFM

/*** sound_ym2612fetch - fetch byte from ym2612 chip ***/

uint8 sound_ym2612fetch(uint8 addr)
{
  return jfm_read(sound_ctx, addr);
}

/*** sound_ym2612store - store a byte to the ym2612 chip ***/

void sound_ym2612store(uint8 addr, uint8 data)
{
  jfm_write(sound_ctx, addr, data);
}

#endif
