/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* sound.c                                                                   */
/*                                                                           */
/*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>

#include "generator.h"
#include "gensound.h"
#include "vdp.h"
#include "ui.h"

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

#include <sys/soundcard.h>

/*** variables externed ***/

int sound_feedback = 0; /* -1, running out of sound
			   +0, lots of sound, do something */
unsigned int sound_threshold; /* bytes in buffer we're trying to maintain */

/* SN76489 snd_cpu; */

/*** forward references ***/

/* void snd_sndout(int chan, int freq, int vol); */

/*** file scoped variables ***/

#define SOUND_MAXRATE 44100
#define SOUND_FRAGMENTS 12
#define SOUND_THRESHOLD 10

static unsigned int sound_sampsperfield = 0;
static int sound_dev = 0;
/* static int sound_dump = 0; */
static uint16 soundbuf[2][SOUND_MAXRATE/50]; /* pal is lowest framerate */
static int sound_format;
static int sound_stereo;
static int sound_speed;
static int sound_frag;
static int sound_fragsize;
static int sound_inited = 0; /* flag to say whether we've inited */

#ifdef JFM
  static t_jfm_ctx *sound_ctx;
#endif

/*** sound_init - initialise this sub-unit ***/

int sound_init(void)
{
  audio_buf_info sound_info;

  LOG_NORMAL(("Initialising sound..."));
  sound_sampsperfield = SOUND_SAMPLERATE / vdp_framerate;

  /* sound_dump = open("/tmp/sound_dump", O_CREAT|O_WRONLY|O_TRUNC); */

  if ((sound_dev = open(SOUND_DEVICE, O_WRONLY, 0)) == -1) {
    LOG_CRITICAL(("open " SOUND_DEVICE " failed: %s", strerror(errno)));
    return 1;
  }
  sound_frag = (SOUND_FRAGMENTS<<16 |
		(int)(ceil(log10(sound_sampsperfield*4)/log10(2))));
  if (ioctl(sound_dev, SNDCTL_DSP_SETFRAGMENT, &sound_frag) == -1) {
    LOG_CRITICAL(("Error setting fragment size: %s", strerror(errno)));
    return 1;
  }
  sound_format = AFMT_S16_LE;
  if (ioctl(sound_dev, SNDCTL_DSP_SETFMT, &sound_format) == -1) {
    LOG_CRITICAL(("Error setting sound device format: %s", strerror(errno)));
    return 1;
  }
  if (sound_format != AFMT_S16_LE && sound_format != AFMT_S16_BE) {
    LOG_CRITICAL(("Sound device format not supported (must be 16 bit)"));
    return 1;
  }
  sound_stereo = 1;
  if (ioctl(sound_dev, SNDCTL_DSP_STEREO, &sound_stereo) == -1) {
    LOG_CRITICAL(("Error setting stereo: %s", strerror(errno)));
    return 1;
  }
  if (sound_stereo != 1) {
    LOG_CRITICAL(("Sound device does not support stereo"));
    return 1;
  }
  sound_speed = SOUND_SAMPLERATE;
  if (ioctl(sound_dev, SNDCTL_DSP_SPEED, &sound_speed) == -1) {
    LOG_CRITICAL(("Error setting sound speed: %s",  strerror(errno)));
    return 1;
  }
  if (sound_speed != SOUND_SAMPLERATE) {
    if (abs(sound_speed-SOUND_SAMPLERATE) > 50) {
      LOG_NORMAL(("Warning: Sample rate not exactly 22050"));
    } else {
      LOG_CRITICAL(("Sound device does not support sample rate %d "
		    "(returned %d)", SOUND_SAMPLERATE, sound_speed));
      return 1;
    }
  }
  if (ioctl(sound_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1) {
    LOG_CRITICAL(("Error getting output space info", strerror(errno)));
    return 1;
  }
  LOG_NORMAL(("Total allocated fragments = %d (requested %d)",
	      sound_info.fragstotal, SOUND_FRAGMENTS));
  LOG_NORMAL(("Fragment size = %d (requested %d)", sound_info.fragsize,
	      sound_sampsperfield*4));
  LOG_NORMAL(("Bytes free in buffer = %d", sound_info.bytes));
  sound_threshold = sound_sampsperfield*4*SOUND_THRESHOLD;
  LOG_NORMAL(("Theshold = %d (%d fields of sound === %dms latency)",
	      sound_threshold, SOUND_THRESHOLD,
	      (int)(1000*(float)SOUND_THRESHOLD/(float)vdp_framerate)));
  if ((signed)sound_threshold >= sound_info.bytes) {
    LOG_CRITICAL(("Threshold exceeds bytes free in buffer"));
    return 1;
  }
#ifdef JFM
  if ((sound_ctx = jfm_init(0, 2612, vdp_clock/7, sound_speed,
			    NULL, NULL)) == NULL)
#else
  if (YM2612Init(1, vdp_clock/7, sound_speed, NULL, NULL))
#endif
    return 1;
  LOG_NORMAL(("YM2612 Initialised @ sample rate %d", sound_speed));
  return 0;
}

/*** sound_final - finalise this sub-unit ***/

void sound_final(void)
{
#ifdef JFM
  jfm_final(sound_ctx);
#else
  YM2612Shutdown();
#endif
  if (sound_dev)
    close(sound_dev);
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
  unsigned int i;
  uint16 buffer[(SOUND_MAXRATE/50)*2]; /* pal is slowest framerate */
#ifdef WORDS_BIGENDIAN
  int endian = 1;
#else
  int endian = 0;
#endif
  audio_buf_info sound_info;
  unsigned int pending;

  if (ioctl(sound_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1)
    ui_err("Error getting output space info", strerror(errno));
  pending = (sound_info.fragstotal*sound_info.fragsize)-sound_info.bytes;
  if (pending < sound_threshold)
    sound_feedback = -1;
  else
    sound_feedback = 0;
  LOG_DEBUG1(("END FIELD! 0 to %d (%d)", sound_sampsperfield, sound_feedback));
  if (sound_format == AFMT_S16_LE && endian == 0) {
    /* device matches endianness of host */
    for (i = 0; i < sound_sampsperfield; i++) {
      buffer[i*2] = soundbuf[0][i];
      buffer[i*2+1] = soundbuf[1][i];
    }
  } else {
    /* device is odd and is different to host endianness */
    for (i = 0; i < sound_sampsperfield; i++) {
      buffer[i*2] = (soundbuf[0][i] >> 8) | ((soundbuf[0][i] << 8) & 0xff00);
      buffer[i*2+1] = (soundbuf[1][i] >> 8) | ((soundbuf[1][i] << 8) & 0xff00);
    }
  }
  if (write(sound_dev, buffer, sound_sampsperfield*4) == -1) {
    if (errno != EINTR)
      ui_err("Error writing to sound device: %s", strerror(errno));
  }
  /*
  if (write(sound_dump, buffer, sound_sampsperfield*4) == -1)
    ui_err("Error writing to dump file: %s", strerror(errno));
  */
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
