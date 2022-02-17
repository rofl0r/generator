/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include <sys/ioctl.h>
#include <math.h>

#include "gensound.h"
#include "gensoundp.h"
#include "vdp.h"
#include "ui.h"

#ifdef USE_SDL_AUDIO

#include "SDL.h"

typedef struct {
  Uint32 *samples;
  size_t in; 
  size_t out; 
  size_t fill; /* Number of samples currently queued */
  size_t size; /* Size of buffer in samples */
} soundp_buffer_t;

static soundp_buffer_t soundp_buffer;

void soundp_callback(void *userdata, Uint8 *stream, int length)
{
  soundp_buffer_t *buf = userdata;
  size_t n_samples;
  
  n_samples = (unsigned) length / sizeof(buf->samples[0]);
  if (n_samples > buf->fill) {
#if 0 
    static const char msg[] = "buffer underrun!\n";
    write(STDERR_FILENO, msg, sizeof msg - 1);
#endif
  }

  while (n_samples > 0) {
    Uint32 *last;
    
    if (buf->out >= buf->size) {
      buf->out = 0;
    }
    last = &buf->samples[(buf->out > 0 ? buf->out : buf->size) - 1];
    if (buf->fill > 0) {
      size_t n;

      n = buf->size - buf->out;
      n = MIN(n, buf->fill);
      n = MIN(n, n_samples);
      memcpy(stream, &buf->samples[buf->out], n * sizeof buf->samples[0]);
      buf->out += n;
      buf->fill -= n;
      n_samples -= n;
      stream += n * sizeof buf->samples[0];
    } else {
      /* Repeat the last sample to avoid blips when there's an underrun */
      while (n_samples > 0) {
        n_samples--;
        memcpy(stream, last, sizeof *last);
        stream += sizeof *last;
      }
    }
  }
}

int soundp_start(void)
{
  SDL_AudioSpec as;

  if (!(SDL_WasInit(SDL_INIT_EVERYTHING) & SDL_INIT_AUDIO))
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
      LOG_CRITICAL(("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s",
        SDL_GetError()));
      return 1;
    }

  soundp_buffer.in = 0;
  soundp_buffer.out = 0;
  soundp_buffer.fill = 0;
  soundp_buffer.size = sound_sampsperfield * sound_maxfields * 2;
  soundp_buffer.samples = calloc(1,
    soundp_buffer.size * sizeof soundp_buffer.samples[0]);
  if (!soundp_buffer.samples) {
    LOG_CRITICAL(("Cannot allocate memory for audio buffer: %s",
      strerror(errno)));
    return 1;
  }

  as.userdata = &soundp_buffer;
  as.freq = sound_speed;
  as.format = AUDIO_S16;
  as.channels = 2;
  as.samples = 1 <<
    (int)(ceil(log10(sound_minfields * sound_sampsperfield) / log10(2)));
  printf("%s: samples=%d\n", __func__, (int)as.samples);
  as.callback = soundp_callback;

  if (SDL_OpenAudio(&as, NULL) == -1) {
    LOG_CRITICAL(("SDL_OpenAudio failed: %s", SDL_GetError()));
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 1;
  }

  SDL_PauseAudio(0);
  LOG_NORMAL(("Audio initialised"));
  return 0;
}

void soundp_stop(void)
{
  if (SDL_WasInit(SDL_INIT_AUDIO))
  {
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
  if (soundp_buffer.samples)
    free(soundp_buffer.samples);
  soundp_buffer.samples = NULL;
  soundp_buffer.in = 0;
  soundp_buffer.out = 0;
  soundp_buffer.fill = 0;
  soundp_buffer.size = 0;
}

int soundp_samplesbuffered(void)
{
  int pending = -1;

  if (soundp_buffer.samples != NULL) {
    SDL_LockAudio();
    pending = soundp_buffer.fill;
    SDL_UnlockAudio();
  }

  return pending;
}

int soundp_output(uint16 *left, uint16 *right, unsigned int samples)
{
  size_t i;

  if (!soundp_buffer.samples)
    return -1;

  SDL_LockAudio();

  if (samples > soundp_buffer.size - soundp_buffer.fill) {
#if 0
    LOG_CRITICAL(("Too many samples passed to soundp_output!"));
#endif
    SDL_UnlockAudio();
    return -1;
  }

  if (samples > 0 && soundp_buffer.fill < soundp_buffer.size) {
    size_t len;

    len = soundp_buffer.size - soundp_buffer.fill;
    len = MIN(len, samples);

    for (i = 0; i < len; i++) {
#ifdef WORDS_BIGENDIAN
      soundp_buffer.samples[soundp_buffer.in++] = (left[i] << 16)| right[i];
#else
      soundp_buffer.samples[soundp_buffer.in++] = right[i] | (left[i] << 16);
#endif
      if (soundp_buffer.in >= soundp_buffer.size)
        soundp_buffer.in = 0;
    }

    soundp_buffer.fill += len;
    samples -= len;
  }
 
  SDL_UnlockAudio();
  return 0;
}

#else /* !USE_SDL_AUDIO */

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#else

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#endif /* HAVE_SOUNDCARD_H */

#if defined(__NetBSD__) || defined(__OpenBSD__)
#define SOUND_DEVICE "/dev/sound"
#else
#define SOUND_DEVICE "/dev/dsp"
#endif /* __NetBSD__ || __OpenBSD__ */

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
      LOG_NORMAL(("Warning: Sample rate not exactly %d", sound_speed));
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

int soundp_output(uint16 *left, uint16 *right, unsigned int samples)
{
  /* pal is slowest framerate */
  uint32 buffer[(SOUND_MAXRATE / 50)], *dst = buffer;
  unsigned int i;

  if (samples > (sizeof buffer << 1)) {
    /* we only bother with coping with one fields worth of samples */
    LOG_CRITICAL(("Too many samples passed to soundp_output!"));
    return -1;
  }

#ifdef WORDS_BIGENDIAN
  if (soundp_format == AFMT_S16_BE) {
#else  
  if (soundp_format == AFMT_S16_LE) {
#endif  
    /* device matches endianness of host */
    for (i = 0; i < samples; i++) {
#ifdef WORDS_BIGENDIAN
      dst[i] = (left[i] << 16) | right[i];
#else
      dst[i] = left[i] | (right[i] << 16);
#endif
    }
  } else {
    /* device is odd and is different to host endianness */
    for (i = 0; i < samples; i++) {
#ifdef WORDS_BIGENDIAN
      dst[i] = (SWAP16(left[i]) << 16) | SWAP16(right[i]);
#else
      dst[i] = SWAP16(left[i]) | (SWAP16(right[i]) << 16);
#endif
     }
  }
  if (write(soundp_dev, buffer, samples * 4) == -1) {
    if (errno != EINTR && errno != EWOULDBLOCK)
      LOG_CRITICAL(("Error writing to sound device: %s", strerror(errno)));
  }
  return 0;
}
#endif /* USE_SDL_AUDIO */

/* vi: set ai et ts=2 sts=2 sw=2 cindent: */
