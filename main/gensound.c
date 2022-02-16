/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <stdlib.h>

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
unsigned int sound_speed;       /* sample rate */
unsigned int sound_sampsperfield;       /* samples per field */
unsigned int sound_threshold;   /* samples in buffer aiming for */
uint8 sound_regs1[256];
uint8 sound_regs2[256];
uint8 sound_address1 = 0;
uint8 sound_address2 = 0;
uint8 sound_keys[8];

/* pal is lowest framerate */
uint16 sound_soundbuf[2][SOUND_MAXRATE / 50];

/*** forward references ***/

/*** file scoped variables ***/

static int sound_active = 0;

#ifdef JFM
static t_jfm_ctx *sound_ctx;
#endif

/*** sound_init - initialise this sub-unit ***/

int sound_init(void)
{
  int ret;

  /* The sound_minfields parameter specifies how many fields worth of sound we
     should try and keep around (a display field is 1/50th or 1/60th of a
     second) - generator works by trying to keep sound_minfields number of
     fields worth of sound around, and drops plotting frames to keep up on slow
     machines */

  sound_speed = SOUND_SAMPLERATE;
  sound_sampsperfield = sound_speed / vdp_framerate;
  sound_threshold = sound_sampsperfield * sound_minfields;

  ret = sound_start();
  if (ret)
    return ret;
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
  LOG_VERBOSE(("Starting sound..."));
  if (soundp_start() == -1) {
    LOG_VERBOSE(("Failed to start sound hardware"));
    return -1;
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

/*** sound_endfield - end frame and output sound ***/

void sound_endfield(void)
{
  unsigned int i;
  int pending;

  /* work out feedback from sound system */

  if ((pending = soundp_samplesbuffered()) == -1)
    ui_err("Failed to read pending bytes in output sound buffer");
  if ((unsigned int)pending < sound_threshold)
    sound_feedback = -1;
  else
    sound_feedback = 0;

  if (sound_debug) {
    LOG_VERBOSE(("End of field - sound system says %d bytes buffered",
                 pending));
    LOG_VERBOSE(("Threshold %d, therefore feedback = %d ", sound_threshold,
                 sound_feedback));
  }
  soundp_output(sound_soundbuf[0], sound_soundbuf[1], sound_sampsperfield);
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

/*** sound_sn76496store - store a byte to the sn76496 chip ***/

void sound_sn76496store(uint8 data)
{
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

/*** sound_process - process sound ***/

void sound_process(void)
{
  static sint16 *tbuf[2];
  int s1 = (sound_sampsperfield * (vdp_line)) / vdp_totlines;
  int s2 = (sound_sampsperfield * (vdp_line + 1)) / vdp_totlines;
  /* pal is lowest framerate */
  uint16 sn76496buf[SOUND_MAXRATE / 50];        /* far too much but who cares */
  unsigned int samples = s2 - s1;
  unsigned int i;

  tbuf[0] = sound_soundbuf[0] + s1;
  tbuf[1] = sound_soundbuf[1] + s1;

  if (s2 > s1) {
#ifdef JFM
    jfm_update(sound_ctx, (void **)tbuf, samples1);
#else
    YM2612UpdateOne(0, (void **)tbuf, samples);
#endif
    SN76496Update(0, sn76496buf, samples);

    /* SN76496 outputs sound in range -0x4000 to 0x3fff
       YM2612 ouputs sound in range -0x8000 to 0x7fff - therefore
       we take 3/4 of the YM2612 and add half the SN76496 */
    for (i = 0; i < samples; i++) {
      sint16 snsample = sn76496buf[i] - 0x4000;
      sint32 l = (tbuf[0][i] * 3) >> 2; /* left channel */
      sint32 r = (tbuf[1][i] * 3) >> 2; /* right channel */
      l += snsample >> 1;
      r += snsample >> 1;
      /* write with clipping
         tbuf[0][i] = l > 0x7FFF ? 0x7fff : ((l < -0x7fff) ? -0x7fff : l);
         tbuf[1][i] = r > 0x7FFF ? 0x7fff : ((r < -0x7fff) ? -0x7fff : r); */
      tbuf[0][i] = l;
      tbuf[1][i] = r;
    }
  }
}
