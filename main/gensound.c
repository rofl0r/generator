/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include "gensound.h"
#include "gensoundp.h"
#include "vdp.h"
#include "ui.h"
#include "sn76496.h"

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

/*** variables externed ***/

int sound_debug = 0;            /* debug mode */
int sound_feedback = 0;         /* -1, running out of sound
                                   +0, lots of sound, do something */
unsigned int sound_minfields = 5;       /* min fields to try to buffer */
unsigned int sound_maxfields = 10;      /* max fields before blocking */
unsigned int sound_speed = SOUND_SAMPLERATE;    /* sample rate */
unsigned int sound_sampsperfield;       /* samples per field */
unsigned int sound_threshold;   /* samples in buffer aiming for */
uint8 sound_regs1[256];
uint8 sound_regs2[256];
uint8 sound_address1 = 0;
uint8 sound_address2 = 0;
uint8 sound_keys[8];
int sound_logsample = 0;        /* sample to log or -1 if none */
unsigned int sound_on = 1;      /* sound enabled */
unsigned int sound_psg = 1;     /* psg enabled */
unsigned int sound_fm = 1;      /* fm enabled */
unsigned int sound_filter = 50; /* low-pass filter percentage (0-100) */

/* pal is lowest framerate */
uint16 sound_soundbuf[2][SOUND_MAXRATE / 50];

/*** forward references ***/

static void sound_process(void);
static void sound_writetolog(unsigned char c);

/*** file scoped variables ***/

static int sound_active = 0;
static uint8 *sound_logdata;    /* log data */
static unsigned int sound_logdata_size; /* sound_logdata size */
static unsigned int sound_logdata_p;    /* current log data offset */
static unsigned int sound_fieldhassamples;      /* flag if field has samples */

#ifdef JFM
static t_jfm_ctx *sound_ctx;
#endif

/*** sound_init - initialise this sub-unit ***/

int sound_init(void)
{
  /* The sound_minfields parameter specifies how many fields worth of sound we
     should try and keep around (a display field is 1/50th or 1/60th of a
     second) - generator works by trying to keep sound_minfields number of
     fields worth of sound around, and drops plotting frames to keep up on slow
     machines */

  sound_threshold = sound_minfields * sound_sampsperfield;

  sound_start();
  
#ifdef JFM
  if ((sound_ctx = jfm_init(0, 2612, vdp_clock / 7, sound_speed,
                            NULL, NULL)) == NULL) {
#else
  if (YM2612Init(1, vdp_clock / 7, sound_speed, NULL, NULL)) {
#endif
    LOG_VERBOSE(("YM2612 failed init"));
    sound_stop();
    return 1;
  }
  if (SN76496Init(0, vdp_clock / 15, 0, sound_speed)) {
    LOG_VERBOSE(("SN76496 failed init"));
    sound_stop();
#ifdef JFM
    jfm_final(sound_ctx);
#else
    YM2612Shutdown();
#endif
    return 1;
  }
  if (sound_logdata)
    free(sound_logdata);
  sound_logdata_size = 8192;
  sound_logdata = calloc(1, sound_logdata_size);
  if (!sound_logdata)
    ui_err("out of memory");
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

/*** sound_start - start sound ***/

int sound_start(void)
{
  if (sound_active)
    sound_stop();
  sound_sampsperfield = sound_speed / vdp_framerate;
  LOG_VERBOSE(("Starting sound..."));
  if (soundp_start() != 0) {
    LOG_CRITICAL(("Failed to start sound hardware"));
    sound_active = 0;
    return 1;
  }
  sound_active = 1;
  LOG_VERBOSE(("Started sound."));
  return 0;
}

/*** sound_stop - stop sound ***/

void sound_stop(void)
{
  if (!sound_active)
    return;
  LOG_VERBOSE(("Stopping sound..."));
  soundp_stop();
  sound_active = 0;
  LOG_VERBOSE(("Stopped sound."));
}

/*** sound_reset - reset sound sub-unit ***/

int sound_reset(void)
{
  sound_final();
  return sound_init();
}

/*** sound_startfield - start of frame ***/

void sound_startfield(void)
{
  sound_logdata_p = 0;
  if (gen_musiclog == musiclog_gnm) {
    sound_writetolog(0);
    sound_writetolog((vdp_totlines >> 8) & 0xff);
    sound_writetolog(vdp_totlines & 0xff);
    sound_fieldhassamples = 0;
  }
}
/*** sound_endfield - end frame and output sound ***/

void sound_endfield(void)
{
  int pending;
  uint8 *p, *o;

  if (gen_musiclog) {
    if (gen_musiclog == musiclog_gym) {
      /* GYM format ends a field with a 0 byte */
      sound_writetolog(0);
    } else {
      /* GNM format requires sifting through the data */
      if (!sound_fieldhassamples) {
        /* we're only removing data, so we're going to write to the buffer
           we're reading from */
        o = sound_logdata + 3;
        for (p = sound_logdata + 3;
             p < (sound_logdata + sound_logdata_p); p++) {
          if ((*p & 0xF0) != 0x00 || *p == 4)
            ui_err("assertion of no samples failed");
          switch (*p) {
          case 0:
            ui_err("field marker in middle of field data");
          case 1:
          case 2:
            /* FM data, copy 2 and two bytes to output */
            *o++ = *p++;
            *o++ = *p++;
            *o++ = *p;
            break;
          case 3:
            /* PSG data, copy 3 and one byte to output */
            *o++ = *p++;
            *o++ = *p;
            break;
          case 5:
            /* these are the bytes we're trying to strip */
            /* do nothing */
            break;
          default:
            ui_err("invalid data in sound log buffer");
          }
        }
        /* update new end of buffer */
        sound_logdata_p = o - sound_logdata;
        sound_logdata[1] = 0; /* high byte number of samples */
        sound_logdata[2] = 0; /* low byte number of samples */
      }
    }
    /* write data */
    ui_musiclog(sound_logdata, sound_logdata_p);
    /* sound_startfield resets everything */
  }

  /* work out feedback from sound system */

  if (!sound_active) {
    sound_feedback = 0;
    return;
  }

  if ((pending = soundp_samplesbuffered()) == -1) {
    ui_err("Failed to read pending bytes in output sound buffer");
  }
  if ((unsigned int)pending < sound_threshold)
    sound_feedback = -1;
  else {
  /* XXX: I'm not sure why it's necessary to differ. Maybe because SDL uses
   *      an extra audio thread(?). If sound_feedback is (much) bigger than
   *      zero, the process becomes pretty CPU greedy without performing
   *      any better, rather worse, just wasting CPU cycles!
   * 
   * cbiere, 2003-01-13
   */
#ifdef USE_SDL_AUDIO
    /* calculate the time (in microseconds) to sleep */
    sound_feedback = pending * 1000 / sound_sampsperfield;
#else
    sound_feedback = 0;
#endif
  }

  if (sound_debug) {
    LOG_VERBOSE(("End of field - sound system says %d bytes buffered",
                 pending));
    LOG_VERBOSE(("Threshold %d, therefore feedback = %d ", sound_threshold,
                 sound_feedback));
  }
  for (;;) {
    struct timespec ts = { 0, 0 };
    int ret;

    ret = soundp_output(sound_soundbuf[0],
 	    sound_soundbuf[1], sound_sampsperfield);
    if (!ret)
      break;
    nanosleep(&ts, NULL);
    sound_feedback -= sound_sampsperfield;
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
    if (sound_address1 == 0x2a) {
      /* sample data */
      sound_keys[7] = 0;
      sound_logsample = data;
    } else {
      if (gen_musiclog != musiclog_off) {
        /* GYM and GNM */
        sound_writetolog(1);
        sound_writetolog(sound_address1);
        sound_writetolog(data);
      }
    }
    if (sound_address1 == 0x28 && (data & 3) != 3)
      sound_keys[data & 7] = data >> 4;
    if (sound_address1 == 0x2b)
      sound_keys[7] = data & 0x80 ? 0xf : 0;
    sound_regs1[sound_address1] = data;
    break;
  case 2:
    if (gen_musiclog != musiclog_off) {
      /* GYM and GNM */
      sound_writetolog(2);
      sound_writetolog(sound_address2);
      sound_writetolog(data);
    }
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

/*** sound_sn76496store - store a byte to the sn76496 chip ***/

void sound_sn76496store(uint8 data)
{
  if (gen_musiclog != musiclog_off) {
    /* GYM and GNM */
    sound_writetolog(3);
    sound_writetolog(data);
  }
  SN76496Write(0, data);
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

/*** sound_line - called at end of line ***/

void sound_line(void)
{
  if (gen_musiclog == musiclog_gnm) {
    /* GNM log */
    if (sound_logsample == -1) {
      sound_writetolog(5);
    } else {
      if ((sound_logsample & 0xF0) == 0) {
        sound_writetolog(4);
      }
      sound_writetolog(sound_logsample);
      sound_logsample = -1;
      /* mark the fact that we have encountered a sample this field */
      sound_fieldhassamples = 1;
    }
  }
  sound_process();
}

/*** sound_process - process sound ***/

static void sound_process(void)
{
  static sint16 *tbuf[2];
  int s1 = (sound_sampsperfield * (vdp_line)) / vdp_totlines;
  int s2 = (sound_sampsperfield * (vdp_line + 1)) / vdp_totlines;
  /* pal is lowest framerate */
  uint16 sn76496buf[SOUND_MAXRATE / 50];        /* far too much but who cares */
  unsigned int samples = s2 - s1;
  unsigned int i;
  static sint32 ll, rr;
  uint32 factora = (0x10000 * sound_filter) / 100;
  uint32 factorb = 0x10000 - factora;

  tbuf[0] = cast_to_void_ptr(&sound_soundbuf[0][s1]);
  tbuf[1] = cast_to_void_ptr(&sound_soundbuf[1][s1]);

  if (s2 > s1) {
    if (sound_fm)
#ifdef JFM
      jfm_update(sound_ctx, (void **)tbuf, samples1);
#else
      YM2612UpdateOne(0, tbuf, samples);
#endif

    /* SN76496 outputs sound in range -0x4000 to 0x3fff
       YM2612 ouputs sound in range -0x8000 to 0x7fff - therefore
       we take 3/4 of the YM2612 and add half the SN76496 */

    /* this uses a single-pole low-pass filter (6 dB/octave) curtesy of
       Jon Watte <hplus@mindcontrol.org> */

    if (sound_fm && sound_psg) {
      SN76496Update(0, sn76496buf, samples);
      for (i = 0; i < samples; i++) {
        sint16 snsample = ((sint16)sn76496buf[i] - 0x4000) >> 1;
        sint32 l = snsample + ((tbuf[0][i] * 3) >> 2); /* left channel */
        sint32 r = snsample + ((tbuf[1][i] * 3) >> 2); /* right channel */
        ll = (ll >> 16) * factora + (l * factorb);
        rr = (rr >> 16) * factora + (r * factorb);
        tbuf[0][i] = ll >> 16;
        tbuf[1][i] = rr >> 16;
      }
    } else if (!sound_fm && !sound_psg) {
      /* no sound */
      memset(tbuf[0], 0, 2 * samples);
      memset(tbuf[1], 0, 2 * samples);
    } else if (sound_fm) {
      /* fm only */
      for (i = 0; i < samples; i++) {
        sint32 l = (tbuf[0][i] * 3) >> 2; /* left channel */
        sint32 r = (tbuf[1][i] * 3) >> 2; /* right channel */
        ll = (ll >> 16) * factora + (l * factorb);
        rr = (rr >> 16) * factora + (r * factorb);
        tbuf[0][i] = ll >> 16;
        tbuf[1][i] = rr >> 16;
      }
    } else {
      /* psg only */
      SN76496Update(0, sn76496buf, samples);
      for (i = 0; i < samples; i++) {
        sint16 snsample = ((sint16)(sn76496buf[i] - 0x4000)) >> 1;
        ll = (ll >> 16) * factora + (snsample * factorb);
        rr = (rr >> 16) * factora + (snsample * factorb);
        tbuf[0][i] = ll >> 16;
        tbuf[1][i] = rr >> 16;
      }
    }
  }
}

/*** sound_writetolog - write to music log buffer ***/

static void sound_writetolog(unsigned char c)
{
  /* write the byte to the self-expanding memory log - when we have the whole
     field's worth of data we then sift through the data (see the
     documentation on the GNM log format) and then pass it to ui_writelog */

  sound_logdata[sound_logdata_p++] = c;
  if (sound_logdata_p >= sound_logdata_size) {
    LOG_VERBOSE(("sound log buffer limit increased"));
    sound_logdata_size+= 8192;
    sound_logdata = realloc(sound_logdata, sound_logdata_size);
    if (!sound_logdata)
      ui_err("out of memory");
  }
}
