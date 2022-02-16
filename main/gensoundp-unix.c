/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "generator.h"
#include "gensound.h"
#include "gensoundp.h"
#include "vdp.h"
#include "ui.h"

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
  #include <soundcard.h>
  #define SOUND_DEVICE "/dev/sound"
#else
  #include <sys/soundcard.h>
  #define SOUND_DEVICE "/dev/dsp"
#endif

/*** variables externed ***/

/*** forward references ***/

/*** file scoped variables ***/

static int soundp_dev = -1;
static int soundp_format;
static int soundp_stereo;
static int soundp_frag;
static unsigned int soundp_speed;

#ifdef JFM
static t_jfm_ctx *sound_ctx;
#endif

/*** soundp_start - start sound hardware ***/

int soundp_start(void)
{
  audio_buf_info sound_info;

  if ((soundp_dev = open(SOUND_DEVICE, O_WRONLY, 0)) == -1) {
    LOG_CRITICAL(("open " SOUND_DEVICE " failed: %s", strerror(errno)));
    return 1;
  }
  soundp_frag = (sound_maxfields << 16 |
                 (int)(ceil(log10(sound_sampsperfield * 4) / log10(2))));
  if (ioctl(soundp_dev, SNDCTL_DSP_SETFRAGMENT, &soundp_frag) == -1) {
    LOG_CRITICAL(("Error setting fragment size: %s", strerror(errno)));
    close(soundp_dev);
    return 1;
  }
#ifdef WORDS_BIGENDIAN
  soundp_format = AFMT_S16_BE;
#else
  soundp_format = AFMT_S16_LE;
#endif
  if (ioctl(soundp_dev, SNDCTL_DSP_SETFMT, &soundp_format) == -1) {
    LOG_CRITICAL(("Error setting sound device format: %s", strerror(errno)));
    close(soundp_dev);
    return 1;
  }
  if (soundp_format != AFMT_S16_LE && soundp_format != AFMT_S16_BE) {
    LOG_CRITICAL(("Sound device format not supported (must be 16 bit)"));
    close(soundp_dev);
    return 1;
  }
  soundp_stereo = 1;
  if (ioctl(soundp_dev, SNDCTL_DSP_STEREO, &soundp_stereo) == -1) {
    LOG_CRITICAL(("Error setting stereo: %s", strerror(errno)));
    close(soundp_dev);
    return 1;
  }
  if (soundp_stereo != 1) {
    LOG_CRITICAL(("Sound device does not support stereo"));
    close(soundp_dev);
    return 1;
  }
  soundp_speed = sound_speed;
  if (ioctl(soundp_dev, SNDCTL_DSP_SPEED, &soundp_speed) == -1) {
    LOG_CRITICAL(("Error setting sound speed: %s", strerror(errno)));
    close(soundp_dev);
    return 1;
  }
  if (soundp_speed != sound_speed) {
    if (abs(soundp_speed - sound_speed) < 100) {
      LOG_NORMAL(("Warning: Sample rate not exactly 22050"));
    } else {
      LOG_CRITICAL(("Sound device does not support sample rate %d "
                    "(returned %d)", sound_speed, soundp_speed));
      close(soundp_dev);
      return 1;
    }
  }
  if (ioctl(soundp_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1) {
    LOG_CRITICAL(("Error getting output space info", strerror(errno)));
    close(soundp_dev);
    return 1;
  }
  LOG_VERBOSE(("Total allocated fragments = %d (requested %d)",
               sound_info.fragstotal, sound_maxfields));
  LOG_VERBOSE(("Fragment size = %d (requested %d)", sound_info.fragsize,
               sound_sampsperfield * 4));
  LOG_VERBOSE(("Bytes free in buffer = %d", sound_info.bytes));
  LOG_VERBOSE(("Theshold = %d bytes (%d fields of sound === %dms latency)",
               sound_threshold * 4, sound_minfields,
               (int)(1000 * (float)sound_minfields / (float)vdp_framerate)));
  if ((signed)sound_threshold >= sound_info.bytes) {
    LOG_CRITICAL(("Threshold exceeds bytes free in buffer"));
    close(soundp_dev);
    return 1;
  }
  return 0;
}

/*** soundp_stop - stop sound hardware ***/

void soundp_stop(void)
{
  if (soundp_dev != -1)
    close(soundp_dev);
  soundp_dev = -1;
}

/*** sound_samplesbuffered - how many samples are currently buffered? ***/

int soundp_samplesbuffered(void)
{
  audio_buf_info sound_info;
  int pending;

  if (ioctl(soundp_dev, SNDCTL_DSP_GETOSPACE, &sound_info) == -1) {
    LOG_CRITICAL(("Error getting output space info: %s", strerror(errno)));
    return -1;
  }
  pending = (sound_info.fragstotal * sound_info.fragsize) - sound_info.bytes;
  return (pending / 4);         /* 2 bytes per sample, 2 channel stereo */
}

void soundp_output(uint16 *left, uint16 *right, unsigned int samples)
{
  uint16 buffer[(SOUND_MAXRATE / 50) * 2];      /* pal is slowest framerate */
#ifdef WORDS_BIGENDIAN
  int endian = 1;
#else
  int endian = 0;
#endif
  unsigned int i;

  if (samples > (sizeof(buffer) << 1)) {
    /* we only bother with coping with one fields worth of samples */
    LOG_CRITICAL(("Too many samples passed to soundp_output!"));
    return;
  }
#ifdef WORDS_BIGENDIAN
  if (soundp_format == AFMT_S16_BE) {
#else  
  if (soundp_format == AFMT_S16_LE) {
#endif  
    /* device matches endianness of host */
    for (i = 0; i < samples; i++) {
      buffer[i * 2] = left[i];
      buffer[i * 2 + 1] = right[i];
    }
  } else {
    /* device is odd and is different to host endianness */
    for (i = 0; i < samples; i++) {
      buffer[i * 2] = ((left[i] >> 8) | ((left[i] << 8) & 0xff00));
      buffer[i * 2 + 1] = ((right[i] >> 8) | ((right[i] << 8) & 0xff00));
    }
  }
  if (write(soundp_dev, buffer, samples * 4) == -1) {
    if (errno != EINTR && errno != EWOULDBLOCK)
      LOG_CRITICAL(("Error writing to sound device: %s", strerror(errno)));
  }
}
