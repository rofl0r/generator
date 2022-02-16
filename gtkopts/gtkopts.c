/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* gtk options loading/saving */

#define _GNU_SOURCE 1
#define _BSD_SOURCE 1
#define __EXTENSIONS__ 1

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "generator.h"
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
  { "view", "100, 200", "100",
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
  { "hborder", "0..32", "8",
    "horizontal over-scan border surrounding main playing area" },
  { "vborder", "0..32", "8",
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
  { "aviframeskip", "integer", "2",
    "AVI forced skip, e.g. 2 is 30fps" },
  { "aviformat", "rgb, jpeg", "jpeg",
    "Type of output to be written to AVI" },
  { "jpegquality", "0-100", "100",
    "JPEG quality - 100 is least compression but best picture" },
  { "lowpassfilter", "0-100", "50",
    "Low-pass sound filter - 0 turns it off, 100 filters too much" },
  { NULL, NULL, NULL, NULL }
};

/* *INDENT-ON* */

int gtkopts_load(const char *file)
{
  char buffer[CONFLINELEN];
  FILE *fd;
  char *a, *p, *q, *t;
  int line = 0;
  t_conf *conf, *confi;

  if ((fd = fopen(file, "r")) == NULL) {
    fprintf(stderr, "%s: unable to open conf '%s' for reading: %s\n",
            PACKAGE, file, strerror(errno));
    return -1;
  }
  while (fgets(buffer, CONFLINELEN, fd)) {
    line++;
    if (buffer[strlen(buffer) - 1] != '\n') {
      fprintf(stderr, "%s: line %d too long in conf file, ignoring.\n",
              PACKAGE, line);
      while ((a = fgets(buffer, CONFLINELEN, fd))) {
        if (buffer[strlen(buffer) - 1] == '\n')
          continue;
      }
      if (!a)
        goto finished;
      continue;
    }
    buffer[strlen(buffer) - 1] = '\0';
    /* remove whitespace off start and end */
    p = buffer + strlen(buffer) - 1;
    while (p > buffer && *p == ' ')
      *p-- = '\0';
    p = buffer;
    while (*p == ' ')
      p++;
    /* check for comment */
    if (!*p || *p == '#' || *p == ';')
      continue;
    if ((conf = malloc(sizeof(t_conf))) == NULL) {
      fprintf(stderr, "%s: Out of memory adding to conf\n", PACKAGE);
      goto error;
    }
    q = p;
    strsep(&q, "=");
    if (q == NULL) {
      fprintf(stderr, "%s: line %d not understood in conf file\n", PACKAGE,
              line);
      goto error;
    }
    /* remove whitespace off end of p and start of q */
    t = p + strlen(p) - 1;
    while (t > p && *t == ' ')
      *t-- = '\0';
    while (*q == ' ')
      q++;
    if ((conf->key = malloc(strlen(p) + 1)) == NULL ||
        (conf->value = malloc(strlen(q) + 1)) == NULL) {
      fprintf(stderr, "%s: Out of memory building conf\n", PACKAGE);
      goto error;
    }
    strcpy(conf->key, p);
    strcpy(conf->value, q);
    conf->next = NULL;
    for (confi = gtkopts_conf; confi && confi->next; confi = confi->next);
    if (!confi)
      gtkopts_conf = conf;
    else
      confi->next = conf;
  }
finished:
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
