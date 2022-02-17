/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* gtk options loading/saving */

#define _GNU_SOURCE 1
#define _BSD_SOURCE 1
#define __EXTENSIONS__ 1

#include "generator.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "gtkopts.h"

t_conf *gtkopts_conf = NULL;

static const char *gtkopts_default(const char *key);

#define CONFLINELEN 1024

typedef struct {
  char *key;
  char *vals;
  char *def;
  char *desc;
} t_opts;

/* *INDENT-OFF* */

static t_opts gtkopts_opts[] = {
  { "view", "100, 200, fullscreen", "100",
    "view mode to use in percent" },
  { "region", "domestic, overseas", "overseas",
    "hardware region" },
  { "videostd", "ntsc, pal", "ntsc",
    "video standard" },
  { "autodetect", "on, off", "on",
    "automatic region, videostd detection" },
  { "plotter", "line, cell", "line",
    "plotter style - line is slower but much more accurate" },
  { "interlace", "bob, weave, weave-filter", "weave-filter",
    "style of de-interlacing to use in 200% view mode" },
  { "frameskip", "auto, 1..10", "auto",
    "skip frames to keep up - auto setting needs sound turned on" },
  { "hborder", "0..32", "0",
    "horizontal over-scan border surrounding main playing area" },
  { "vborder", "0..32", "0",
    "vertical retrace border surrounding main playing area" },
  { "sound", "on, off", "on",
    "sound system toggle, also turns off fm and psg (not z80)" },
  { "z80", "on, off", "on",
    "z80 cpu" },
  { "fm", "on, off", "on",
    "frequency modulation sound generation" },
  { "psg", "on, off", "on",
    "programmable sound generation" },
  { "sound_minfields", "integer", "5",
    "try to buffer this many fields of sound" },
  { "sound_maxfields", "integer", "10",
    "maximum buffered sound fields before blocking (waiting)" },
  { "loglevel", "integer (0-7)", "1", 
    "logging level" },
  { "debugsound", "on, off", "off",
    "extra sound debug logging" },
  { "statusbar", "on, off", "off",
    "status bar on main window (fps counter)" },
  { "key1_a", "gdk keysym", "a",
    "keyboard player 1 'A' button" },
  { "key1_b", "gdk keysym", "s",
    "keyboard player 1 'B' button" },
  { "key1_c", "gdk keysym", "d",
    "keyboard player 1 'C' button" },
  { "key1_up", "gdk keysym", "Up",
    "keyboard player 1 up" },
  { "key1_down", "gdk keysym", "Down",
    "keyboard player 1 down" },
  { "key1_left", "gdk keysym", "Left",
    "keyboard player 1 left" },
  { "key1_right", "gdk keysym", "Right",
    "keyboard player 1 right" },
  { "key1_start", "gdk keysym", "Return",
    "keyboard player 1 start button" },
  { "key2_a", "gdk keysym", "KP_Divide",
    "keyboard player 2 'A' button" },
  { "key2_b", "gdk keysym", "KP_Multiply",
    "keyboard player 2 'B' button" },
  { "key2_c", "gdk keysym", "KP_Subtract",
    "keyboard player 2 'C' button" },
  { "key2_up", "gdk keysym", "KP_8",
    "keyboard player 2 up" },
  { "key2_down", "gdk keysym", "KP_5",
    "keyboard player 2 down" },
  { "key2_left", "gdk keysym", "KP_4",
    "keyboard player 2 left" },
  { "key2_right", "gdk keysym", "KP_6",
    "keyboard player 2 right" },
  { "key2_start", "gdk keysym", "KP_Enter",
    "keyboard player 2 start button" },

  { "joy1_a", "0..255", "0",
    "joypad player 1 'A' button" },
  { "joy1_b", "0..255", "1",
    "joypad player 1 'B' button" },
  { "joy1_c", "0..255", "2",
    "joypad player 1 'C' button" },
  { "joy1_start", "0..255", "3",
    "joypad player 1 start button" },
  { "joy1_up", "0..255", "4",
    "joypad player 1 up button" },
  { "joy1_down", "0..255", "5",
    "joypad player 1 down button" },
  { "joy1_left", "0..255", "6",
    "joypad player 1 left button" },
  { "joy1_right", "0..255", "7",
    "joypad player 1 right button" },

  { "joy2_a", "0..255", "0",
    "joypad player 2 'A' button" },
  { "joy2_b", "0..255", "1",
    "joypad player 2 'B' button" },
  { "joy2_c", "0..255", "2",
    "joypad player 2 'C' button" },
  { "joy2_start", "0..255", "3",
    "joypad player 2 start button" },
  { "joy2_up", "0..255", "4",
    "joypad player 2 up button" },
  { "joy2_down", "0..255", "5",
    "joypad player 2 down button" },
  { "joy2_left", "0..255", "6",
    "joypad player 2 left button" },
  { "joy2_right", "0..255", "7",
    "joypad player 2 right button" },

  { "aviframeskip", "integer", "2",
    "AVI forced skip, e.g. 2 is 30fps" },
  { "aviformat", "rgb, jpeg", "jpeg",
    "Type of output to be written to AVI" },
  { "jpegquality", "0-100", "100",
    "JPEG quality - 100 is least compression but best picture" },
  { "lowpassfilter", "0-100", "50",
    "Low-pass sound filter - 0 turns it off, 100 filters too much" },
  { "samplerate", "11025, 22050, 44100, 48000", "44100",
    "Samplerate to use for audio" },
  { NULL, NULL, NULL, NULL }
};

/* *INDENT-ON* */

int gtkopts_load(const char *file)
{
  char buffer[CONFLINELEN];
  FILE *fd;
  int line = 0;
  t_conf **conf_iter = &gtkopts_conf;
  int truncated = 0;

  if ((fd = fopen(file, "r")) == NULL) {
    fprintf(stderr, "%s: unable to open conf '%s' for reading: %s\n",
            PACKAGE, file, strerror(errno));
    return -1;
  }
  while (fgets(buffer, CONFLINELEN, fd)) {
    t_conf *conf;
    char *p, *q, *t;
    size_t len;

    line++;

    len = strlen(buffer);
    if (0 == len || buffer[len - 1] != '\n') {
      fprintf(stderr, "%s: line %d too long in conf file, ignoring.\n",
              PACKAGE, line);
      truncated = 1;
      continue;
    } else if (truncated) {
      truncated = 0;
      continue;
    }

    /* remove whitespace off end */
    p = &buffer[len - 1];
    do {
      if (!isspace((unsigned char) *p))
        break;
      *p = '\0';
    } while (p-- != buffer);

    /* remove whitespace off start */
    p = buffer;
    while (isspace((unsigned char) *p))
      p++;

    /* check for comment */
    if (!*p || *p == '#' || *p == ';')
      continue;
    q = strchr(p, '=');
    if (q == NULL || q == p) {
      fprintf(stderr, "%s: line %d not understood in conf file\n", PACKAGE,
              line);
      goto error;
    }
    t = q;
    *q++ = '\0';
    /* remove whitespace off end of p and start of q */
    while (--t != p && isspace((unsigned char) *t))
      *t = '\0';
    while (isspace((unsigned char) *q))
      q++;

    if ((conf = malloc(sizeof *conf)) == NULL) {
      fprintf(stderr, "%s: Out of memory adding to conf\n", PACKAGE);
      goto error;
    }
    conf->key = strdup(p);
    conf->value = strdup(q);
    conf->next = NULL;
    if (NULL == conf->key || NULL == conf->value) {
      free(conf->key);
      free(conf->value);
      free(conf);
      fprintf(stderr, "%s: Out of memory building conf\n", PACKAGE);
      goto error;
    }

    (*conf_iter) = conf;
    conf_iter = &conf->next;
  }

  if (ferror(fd) || fclose(fd)) {
    fprintf(stderr, "%s: error whilst reading conf: %s\n", PACKAGE,
            strerror(errno));
    return -1;
  }
  return 0;
error:
  fclose(fd);
  return -1;
}

const char *gtkopts_getvalue(const char *key)
{
  t_conf *c;
  const char *v;

  for (c = gtkopts_conf; c; c = c->next) {
    if (!strcasecmp(key, c->key))
      return c->value;
  }
  v = gtkopts_default(key);
  if (!v)
    printf("Warning: invalid option '%s' requested.\n", key);
  return v;
}

int gtkopts_setvalue(const char *key, const char *value)
{
  t_conf *c;
  char *n;

  for (c = gtkopts_conf; c; c = c->next) {
    if (!strcasecmp(key, c->key)) {
      if ((n = malloc(strlen(value) + 1)) == NULL)
        return -1;
      strcpy(n, value);
      free(c->value);
      c->value = n;
      return 0;
    }
  }
  if ((c = malloc(sizeof(t_conf))) == NULL ||
      (c->key = malloc(strlen(key) + 1)) == NULL ||
      (c->value = malloc(strlen(value) + 1)) == NULL)
    return -1;
  strcpy(c->key, key);
  strcpy(c->value, value);
  c->next = gtkopts_conf;
  gtkopts_conf = c;
  return 0;
}

int gtkopts_save(const char *file)
{
  FILE *fd;
  t_opts *o;

  if ((fd = fopen(file, "w")) == NULL) {
    fprintf(stderr, "%s: unable to open conf '%s' for writing: %s\n",
            PACKAGE, file, strerror(errno));
    return -1;
  }
  fprintf(fd, "; Generator " VERSION " configuration file\n\n");
  fprintf(fd, "; for GTK keysyms see include/gtk-1.2/gdk/gdkkeysyms.h\n\n");
  for (o = gtkopts_opts; o->key; o++) {
    fprintf(fd, "; %s\n; values: %s\n", o->desc, o->vals);
    fprintf(fd, "%s = %s\n\n", o->key, gtkopts_getvalue(o->key));
  }
  if (fclose(fd)) {
    fprintf(stderr, "%s: error whilst writing conf: %s\n", PACKAGE,
            strerror(errno));
    return -1;
  }
  return 0;
}

static const char *gtkopts_default(const char *key)
{
  t_opts *o;

  for (o = gtkopts_opts; o->key; o++) {
    if (!strcasecmp(key, o->key))
      return o->def;
  }
  return NULL;
}
