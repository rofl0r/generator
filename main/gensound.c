/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
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

int sound_feedback = 0;           /* -1, running out of sound
                                     +0, lots of sound, do something */
int sound_debug = 0;              /* 0 for no debug, 1 for debug */
int sound_minfields = 5;          /* minimum fields to try to keep buffered */
int sound_maxfields = 10;         /* max fields to buffer before blocking */
uint8 sound_regs1[256];
uint8 sound_regs2[256];
uint8 sound_address1 = 0;
uint8 sound_address2 = 0;
uint8 sound_keys[8];

/* SN76489 snd_cpu; */

/*** forward references ***/

/* void snd_sndout(int chan, int freq, int vol); */

/*** file scoped variables ***/

#define SOUND_MAXRATE 44100
#define SOUND_FRAGMENTS 12

static unsigned int sound_sampsperfield = 0;
static int sound_dev = -1;
/* static int sound_dump = 0; */
static uint16 soundbuf[2][SOUND_MAXRATE/50]; /* pal is lowest framerate */
static int sound_active = 0;
static int sound_format;
static int sound_stereo;
static int sound_speed;
static int sound_frag;
static int sound_fragsize;
static unsigned int sound_threshold; /* bytes in buffer we're trying for */

#ifdef JFM
  static t_jfm_ctx *sound_ctx;
#endif

/*** sound_init - initialise this sub-unit ***/

int sound_init(void)
{
  int ret;

  ret = sound_start();
  if (ret)
    return ret;
#ifdef JFM
  if ((sound_ctx = jfm_init(0, 2612, vdp_clock/7, sound_speed,
			    NULL, NULL)) == NULL) {
#else
  if (YM2612Init(1, vdp_clock/7, sound_speed, NULL, NULL)) {
#endif
    close(sound_dev);
    return 1;
  }
  LOG_VERBOSE(("YM2612 Initialised @ sample rate %d", sound_speed));
  return 0;
}

/*** sound_final - finalise this sub-unit ***/

void sound_final(void)
{
  sound_stop();
#ifdef JFM
  jfm_final(sound_ctx);
#else
  YM2612Shutdown();
#endif
}

/*** sound_start - start sound hardware ***/

int sound_start(void)
{
  audio_buf_info sound_info;

  /* The threshold parameter specifies how many fields worth of sound we
     should try and keep around (a display field is 1/50th or 1/60th of a
     second) - generator works by trying to keep threshold number of fields
     worth of sound around, and drops plotting frames to keep up on slow
     machines */

  if (sound_active)
    sound_stop();
  LOG_VERBOSE(("Initialising sound..."));
  sound_sampsperfield = SOUND_SAMPLERATE / vdp_framerate;

  /* sound_dump = open("/tmp/sound_dump", O_CREAT|O_WRONLY|O_TRUNC); */

  if ((sound_dev = open(SOUND_DEVICE, O_WRONLY, 0)) == -1) {
    LOG_CRITICAL(("open " SOUND_DEVICE " failed: %s", strerror(errno)));
    return 1;
  }
  sound_frag = (sound_maxfields<<16 |
		(int)(ceil(log10(sound_sampsperfield*4)/log10(2))));
  if (ioctl(sound_dev, SNDCTL_DSP_SETFRAGMENT, &sound_frag) == -1) {
    LOG_CRITICAL(("Error setting fragment size: %s", strerror(errno)));
    close(sound_dev);
    return 1;
  }
  sound_format = AFMT_S16_LE;
  if (ioctl(sound_dev, SNDCTL_DSP_SETFMT, &sound_format) == -1) {
    LOG_CRITICAL(("Error setting sound device format: %s", strerror(errno)));
    close(sound_dev);
    return 1;
  }
  if (sound_format != AFMT_S16_LE && sound_format != AFMT_S16_BE) {
    LOG_CRITICAL(("Sound device format not supported (must be 16 bit)"));
    close(sound_dev);
    return 1;
  }
  sound_stereo = 1;
  if (ioctl(sound_dev, SNDCTL_DSP_STEREO, &sound_stereo) == -1) {
    LOG_CRITICAL(("Error setting stereo: %s", strerror(errno)));
    close(sound_dev);
    return 1;
  }
  if (sound_stereo != 1) {
    LOG_CRITICAL(("Sound device does not support stereo"));
    close(sound_dev);
    return 1;
  }
  sound_speed = SOUND_SAMPLERATE;
  if (ioctl(sound_dev, SNDCTL_DSP_SPEED, &sound_speed) == -1) {
    LOG_CRITICAL(("Error setting sound speed: %s",  strerror(errno)));
    close(sound_dev);
    return 1;
  }
  if (sound_speed != SOUND_SAMPLERATE) {
    if (abs(sound_speed-SOUND_SAMPLERATE) > 50) {
      LOG_NORMAL(("Warning: Sample rate not exactly 22050"));
    } else {
      LOG_CRITICAL(("Sound device does not support sample rate %d "
		    "(returned %d)", SOUND_SAMPLERATE, sound_speed));
      close(sound_dev);
      return 1;
    }
  }
  if (ioctl(sound_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1) {
    LOG_CRITICAL(("Error getting output space info", strerror(errno)));
    close(sound_dev);
    return 1;
  }
  LOG_VERBOSE(("Total allocated fragments = %d (requested %d)",
	       sound_info.fragstotal, sound_maxfields));
  LOG_VERBOSE(("Fragment size = %d (requested %d)", sound_info.fragsize,
	       sound_sampsperfield*4));
  LOG_VERBOSE(("Bytes free in buffer = %d", sound_info.bytes));
  sound_threshold = sound_sampsperfield*4*sound_minfields;
  LOG_VERBOSE(("Theshold = %d (%d fields of sound === %dms latency)",
	       sound_threshold, sound_minfields,
	       (int)(1000*(float)sound_minfields/(float)vdp_framerate)));
  if ((signed)sound_threshold >= sound_info.bytes) {
    LOG_CRITICAL(("Threshold exceeds bytes free in buffer"));
    close(sound_dev);
    return 1;
  }
  sound_active = 1;
  return 0;
}
/*** sound_stop - stop sound ***/

void sound_stop(void)
{
  if (!sound_active)
    return;
  LOG_VERBOSE(("Stopping sound..."));
  if (sound_dev != -1)
    close(sound_dev);
  sound_dev = -1;
  sound_active = 0;
  LOG_VERBOSE(("Stopped sound."));
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
  static sint16 *tbuf[2];
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
  struct timeval tv;

  if (ioctl(sound_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1)
    ui_err("Error getting output space info", strerror(errno));
  pending = (sound_info.fragstotal*sound_info.fragsize)-sound_info.bytes;
  if (pending < sound_threshold)
    sound_feedback = -1;
  else
    sound_feedback = 0;
  if (sound_debug) {
    LOG_VERBOSE(("End of field - sound card says output buffer left = %d bytes",
                 sound_info.bytes));
    LOG_VERBOSE(("Sound card says %d frags @ %d bytes each = %d bytes",
                 sound_info.fragstotal, sound_info.fragsize,
                 sound_info.fragstotal*sound_info.fragsize));
    LOG_VERBOSE(("Therefore, pending = %d, compare with threshold %d = %d",
                 pending, sound_threshold, sound_feedback));
  }
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
  if (sound_debug) {
    gettimeofday(&tv, NULL);
    LOG_REQUEST(("%ld:%ld - before write", tv.tv_sec, tv.tv_usec));
  }
  if (write(sound_dev, buffer, sound_sampsperfield*4) == -1) {
    if (errno != EINTR)
      ui_err("Error writing to sound device: %s", strerror(errno));
  }
  /*
  if (write(sound_dump, buffer, sound_sampsperfield*4) == -1)
    ui_err("Error writing to dump file: %s", strerror(errno));
  */
  if (sound_debug) {
    gettimeofday(&tv, NULL);
    LOG_REQUEST(("%ld:%ld - after write", tv.tv_sec, tv.tv_usec));
  }
}

/*** sound_ym2612fetch - fetch byte from ym2612 chip ***/

uint8 sound_ym2612fetch(uint8 addr)
{
#ifdef JFM
  return jfm_read(sound_ctx, addr);
#else
  return YM2612Read(0, addr);
#endif
}

/*** sound_ym2612store - store a byte to the ym2612 chip ***/

void sound_ym2612store(uint8 addr, uint8 data)
{
  switch (addr) {
  case 0:
    sound_address1 = data;
    break;
  case 1:
    if (sound_address1 == 0x28 && (data & 3) != 3)
      sound_keys[data & 7] = data >> 4;
    if (sound_address1 == 0x2a) {
      sound_keys[7] = 0;
    }
    if (sound_address1 == 0x2b)
      sound_keys[7] = data & 0x80 ? 0xf : 0;
    sound_regs1[sound_address1] = data;
    break;
  case 2:
    sound_address2 = data;
    break;
  case 3:
    sound_regs2[sound_address2] = data;
    break;
  }
#ifdef JFM
  jfm_write(sound_ctx, addr, data);
#else
  YM2612Write(0, addr, data);
#endif
}

