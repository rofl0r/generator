/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include <sys/ioctl.h>
#include <math.h>

#include "gensound.h"
#include "gensoundp.h"
#include "vdp.h"
#include "ui.h"

#include <allegro.h>

/*** variables externed ***/

/*** forward references ***/

void sound_readyblock(void);

/*** file scoped variables ***/

static int soundp_format;
static int soundp_stereo;
static int soundp_frag;
static int soundp_fragsize;
static unsigned int soundp_speed;

static unsigned int soundp_block;
static SAMPLE *soundp_sample;
static int soundp_voice;

#ifdef JFM
static t_jfm_ctx *sound_ctx;
#endif

/*** soundp_start - start sound hardware ***/

int soundp_start(void)
{
  unsigned int i;
  uint16 *p;
  static int once = 0;

  if (!once) {
    once = 1;
    LOG_VERBOSE(("Initialising sound..."));
    if (detect_digi_driver(DIGI_AUTODETECT) < 1) {
      LOG_CRITICAL(("Allegro failed to detect hardware"));
      return 1;
    }
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL)) {
      LOG_CRITICAL(("Failed to initialise sound"));
      return 1;
    }
  }
  /* allocate block capable of holding sound_maxfields frames */
  if ((soundp_sample = create_sample(16, 1, sound_speed, sound_sampsperfield *
                                    sound_maxfields)) == NULL) {
    LOG_CRITICAL(("Failed to create sample"));
    return 1;
  }
  p = (uint16 *)soundp_sample->data;
  for (i = 0; i < sound_sampsperfield * sound_maxfields * 2; i--)
    p[i] = 0x8000;
  if ((soundp_voice = allocate_voice(soundp_sample)) == -1) {
    LOG_CRITICAL(("Failed to allocate voice"));
    destroy_sample(soundp_sample);
    return 1;
  }
  voice_set_playmode(soundp_voice, PLAYMODE_LOOP);
  voice_set_volume(soundp_voice, 256);
  voice_set_pan(soundp_voice, 128);
  voice_set_frequency(soundp_voice, sound_speed);
  voice_start(soundp_voice);
  soundp_block = sound_maxfields - 1;       /* next block to write to */
  LOG_VERBOSE(("YM2612 Initialised @ sample rate %d", sound_speed));
  sound_readyblock();
  return 0;
}

/*** soundp_stop - stop sound hardware ***/

void soundp_stop(void)
{
  voice_stop(soundp_voice);
  deallocate_voice(soundp_voice);
  destroy_sample(soundp_sample);
}

/*** soundp_samplesbuffered - how many samples are currently buffered? ***/

int soundp_samplesbuffered(void)
{
  int pos = voice_get_position(soundp_voice);
  unsigned int blockpos = pos / sound_sampsperfield;
  int pending;

  if (blockpos == soundp_block) /* erk, playing in the write block */
    pending = 0;
  else if (blockpos < soundp_block)
    pending = soundp_block - blockpos;
  else
    pending = soundp_block + sound_maxfields - blockpos;
  if (pending) /* quantisation means we've over estimated by one, so adjust */
    pending--;
  return (pending * sound_sampsperfield);
}

void soundp_output(uint16 *left, uint16 *right, unsigned int samples)
{
  int writepos = soundp_block * sound_sampsperfield;
  uint16 *buffer = soundp_sample->data + writepos * 2 * 2;  /* 16bit, stereo */
  unsigned int i;

  if (samples != sound_sampsperfield) {
    LOG_CRITICAL(("allegro sound can only be used in field blocks"));
    return;
  }
  /* write into new block */
  for (i = 0; i < samples; i++) {
    buffer[i * 2] = ((sint16)left[i]) + 0x8000;
    buffer[i * 2 + 1] = ((sint16)right[i]) + 0x8000;
  }
  soundp_block++;
  if (soundp_block >= sound_maxfields)
    soundp_block = 0;
  sound_readyblock();
}

void sound_readyblock(void)
{
  static int locked = 0;
  int writepos = soundp_block * sound_sampsperfield;
  time_t start = time(NULL);

  if (locked && digi_driver->unlock_voice)
    digi_driver->unlock_voice(soundp_voice);

  while ((voice_get_position(soundp_voice) /
          sound_sampsperfield) == soundp_block) {
    /* I used to usleep here, but it breaks on some platforms - then I
       tried uclock() which failed too, so we're just going to busywait */
    if (time(NULL) > start + 10) {
      LOG_CRITICAL(("Sound error - pos=%d %d=%d",
                    voice_get_position(soundp_voice),
                    sound_sampsperfield, soundp_block));
      exit(0);
    }
    /* cannot write to the next block until it's played! */
  }
  if (digi_driver->lock_voice) {
    digi_driver->lock_voice(soundp_voice, writepos,
                            writepos + sound_sampsperfield);
    locked = 1;
  }
}
