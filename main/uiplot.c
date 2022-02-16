/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* plotter routines for user interfaces - used by console and gtk */

#include "generator.h"
#include "vdp.h"

#include "uiplot.h"

uint32 uiplot_palcache[192];

static int uiplot_redshift;
static int uiplot_greenshift;
static int uiplot_blueshift;

void uiplot_setshifts(int redshift, int greenshift, int blueshift)
{
  uiplot_redshift = redshift;
  uiplot_greenshift = greenshift;
  uiplot_blueshift = blueshift;
}

/* uiplot_checkpalcache goes through the CRAM memory in the Genesis and 
   converts it to the uiplot_palcache table.  The Genesis has 64 colours,
   but we store three versions of the colour table into uiplot_palcache - a
   normal, hilighted and dim version.  The vdp_cramf buffer has 64
   entries and is set to 1 when the game writes to CRAM, this means this
   code skips entries that have not changed, unless 'flag' is set to 1 in
   which case this updates all entries regardless. */

void uiplot_checkpalcache(int flag)
{
  unsigned int col;
  uint8 *p;

  /* this code requires that there be at least 4 bits per colour, that
     is, three bits that come from the console's palette, and one more bit
     when we do a dim or bright colour, i.e. this code works with 12bpp
     upwards */

  /* the flag forces it to do the update despite the vdp_cramf buffer */

  for (col = 0; col < 64; col++) {      /* the CRAM has 64 colours */
    if (!flag && !vdp_cramf[col])
      continue;
    vdp_cramf[col] = 0;
    p = (uint8 *)vdp_cram + 2 * col;    /* point p to the two-byte CRAM entry */
    uiplot_palcache[col] =          /* normal */
      (p[0] & 0xE) << (uiplot_blueshift + 1) |
      (p[1] & 0xE) << (uiplot_redshift + 1) |
      ((p[1] & 0xE0) >> 4) << (uiplot_greenshift + 1);
    uiplot_palcache[col + 64] =     /* hilight */
      (p[0] & 0xE) << uiplot_blueshift |
      (p[1] & 0xE) << uiplot_redshift |
      ((p[1] & 0xE0) >> 4) << uiplot_greenshift |
      (16 << uiplot_blueshift) | (16 << uiplot_redshift) |
      (16 << uiplot_greenshift);
    uiplot_palcache[col + 128] =    /* shadow */
      (p[0] & 0xE) << uiplot_blueshift |
      (p[1] & 0xE) << uiplot_redshift |
      ((p[1] & 0xE0) >> 4) << uiplot_greenshift;
  }
}

/*** uiplot_convertdata - convert genesis data to 16 bit colour */

/* must call uiplot_checkpalcache first */

void uiplot_convertdata16(uint8 *indata, uint16 *outdata, unsigned int pixels)
{
  unsigned int ui;
  uint32 outdata1;
  uint32 outdata2;
  uint32 indata_val;

  /* not scaled, 16bpp - we do 4 pixels at a time */
  for (ui = 0; ui < (pixels >> 2); ui++) {
    indata_val = ((uint32 *)indata)[ui];        /* 4 bytes of in data */
    outdata1 = (uiplot_palcache[indata_val & 0xff] |
                uiplot_palcache[(indata_val >> 8) & 0xff] << 16);
    outdata2 = (uiplot_palcache[(indata_val >> 16) & 0xff] |
                uiplot_palcache[(indata_val >> 24) & 0xff] << 16);
#ifdef WORDS_BIGENDIAN
    ((uint32 *)outdata)[(ui << 1)] = outdata2;
    ((uint32 *)outdata)[(ui << 1) + 1] = outdata1;
#else
    ((uint32 *)outdata)[(ui << 1)] = outdata1;
    ((uint32 *)outdata)[(ui << 1) + 1] = outdata2;
#endif
  }
}

/*** uiplot_convertdata - convert genesis data to 32 bit colour ***/

/* must call uiplot_checkpalcache first */

void uiplot_convertdata32(uint8 *indata, uint32 *outdata, unsigned int pixels)
{
  unsigned int ui;
  uint32 outdata1;
  uint32 outdata2;
  uint32 outdata3;
  uint32 outdata4;
  uint32 indata_val;

  /* not scaled, 32bpp - we do 4 pixels at a time */
  for (ui = 0; ui < pixels >> 2; ui++) {
    indata_val = ((uint32 *)indata)[ui];        /* 4 bytes of in data */
    outdata1 = uiplot_palcache[indata_val & 0xff];
    outdata2 = uiplot_palcache[(indata_val >> 8) & 0xff];
    outdata3 = uiplot_palcache[(indata_val >> 16) & 0xff];
    outdata4 = uiplot_palcache[(indata_val >> 24) & 0xff];
#ifdef WORDS_BIGENDIAN
    ((uint32 *)outdata)[(ui << 2)] = outdata4;
    ((uint32 *)outdata)[(ui << 2) + 1] = outdata3;
    ((uint32 *)outdata)[(ui << 2) + 2] = outdata2;
    ((uint32 *)outdata)[(ui << 2) + 3] = outdata1;
#else
    ((uint32 *)outdata)[(ui << 2)] = outdata1;
    ((uint32 *)outdata)[(ui << 2) + 1] = outdata2;
    ((uint32 *)outdata)[(ui << 2) + 2] = outdata3;
    ((uint32 *)outdata)[(ui << 2) + 3] = outdata4;
#endif
  }
}

/*** uiplot_render16_x1 - copy to screen with delta changes (16 bit) ***/

void uiplot_render16_x1(uint16 *linedata, uint16 *olddata, uint8 *screen,
                        unsigned int pixels)
{
  unsigned int ui;
  uint32 inval;

  for (ui = 0; ui < pixels >> 1; ui++) {
    /* do two words of input data at a time */
    inval = ((uint32 *)linedata)[ui];
    if (inval != ((uint32 *)olddata)[ui])
      ((uint32 *)screen)[ui] = inval;
  }
}

/*** uiplot_render32_x1 - copy to screen with delta changes (32 bit) ***/

void uiplot_render32_x1(uint32 *linedata, uint32 *olddata, uint8 *screen,
                        unsigned int pixels)
{
  unsigned int ui;
  uint32 inval;

  for (ui = 0; ui < pixels; ui++) {
    inval = linedata[ui];
    if (inval != olddata[ui])
      ((uint32 *)screen)[ui] = inval;
  }
}

/*** uiplot_render16_x2 - blow up screen image by two (16 bit) ***/

void uiplot_render16_x2(uint16 *linedata, uint16 *olddata, uint8 *screen,
                        unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 inval, outval, mask;
  uint8 *screen2 = screen + linewidth;

  mask = 0xffffffff;
  for (ui = 0; ui < pixels >> 1; ui++) {
    /* do two words of input data at a time */
    inval = ((uint32 *)linedata)[ui];
    if (olddata)
      mask = inval ^ ((uint32 *)olddata)[ui];     /* check for changed data */
    if (mask & 0xffff) {
      /* write first two words */
      outval = inval & 0xffff;
      outval |= outval << 16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui << 1) + 1] = outval;
      ((uint32 *)screen2)[(ui << 1) + 1] = outval;
#else
      ((uint32 *)screen)[(ui << 1)] = outval;
      ((uint32 *)screen2)[(ui << 1)] = outval;
#endif
    }
    if (mask & 0xffff0000) {
      /* write second two words */
      outval = inval & 0xffff0000;
      outval |= outval >> 16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui << 1)] = outval;
      ((uint32 *)screen2)[(ui << 1)] = outval;
#else
      ((uint32 *)screen)[(ui << 1) + 1] = outval;
      ((uint32 *)screen2)[(ui << 1) + 1] = outval;
#endif
    }
  }
}

/*** uiplot_render32_x2 - blow up screen image by two (32 bit) ***/

void uiplot_render32_x2(uint32 *linedata, uint32 *olddata, uint8 *screen,
                        unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 val;
  uint8 *screen2 = screen + linewidth;

  if (olddata) {
    for (ui = 0; ui < pixels; ui++) {
      val = linedata[ui];
      /* check for changed data */
      if (val != olddata[ui]) {
        ((uint32 *)screen)[(ui << 1) + 0] = val;
        ((uint32 *)screen)[(ui << 1) + 1] = val;
        ((uint32 *)screen2)[(ui << 1) + 0] = val;
        ((uint32 *)screen2)[(ui << 1) + 1] = val;
      }
    }
  } else {
    for (ui = 0; ui < pixels; ui++) {
      val = linedata[ui];
      ((uint32 *)screen)[(ui << 1) + 0] = val;
      ((uint32 *)screen)[(ui << 1) + 1] = val;
      ((uint32 *)screen2)[(ui << 1) + 0] = val;
      ((uint32 *)screen2)[(ui << 1) + 1] = val;
    }
  }
}

/*** uiplot_render16_x2h - blow up by two in horizontal direction 
     only (16 bit) ***/

void uiplot_render16_x2h(uint16 *linedata, uint16 *olddata, uint8 *screen,
                         unsigned int pixels)
{
  unsigned int ui;
  uint32 inval, outval, mask;

  mask = 0xffffffff;
  for (ui = 0; ui < pixels >> 1; ui++) {
    /* do two words of input data at a time */
    inval = ((uint32 *)linedata)[ui];
    if (olddata)
      mask = inval ^ ((uint32 *)olddata)[ui];   /* check for changed data */
    if (mask & 0xffff) {
      /* write first two words */
      outval = inval & 0xffff;
      outval |= outval << 16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui << 1) + 1] = outval;
#else
      ((uint32 *)screen)[(ui << 1)] = outval;
#endif
    }
    if (mask & 0xffff0000) {
      /* write second two words */
      outval = inval & 0xffff0000;
      outval |= outval >> 16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui << 1)] = outval;
#else
      ((uint32 *)screen)[(ui << 1) + 1] = outval;
#endif
    }
  }
}

/*** uiplot_render32_x2h - blow up by two in horizontal direction
     only (32 bit) ***/

void uiplot_render32_x2h(uint32 *linedata, uint32 *olddata, uint8 *screen,
                         unsigned int pixels)
{
  unsigned int ui;
  uint32 val;

  for (ui = 0; ui < pixels; ui++) {
    val = linedata[ui];
    /* check for changed data */
    if (!olddata || val != olddata[ui]) {
      ((uint32 *)screen)[(ui << 1) + 0] = val;
      ((uint32 *)screen)[(ui << 1) + 1] = val;
    }
  }
}

/*** uiplot_irender16_weavefilter - take even and odd fields, filter and
     plot (16 bit) ***/

void uiplot_irender16_weavefilter(uint16 *evendata, uint16 *odddata,
                                  uint8 *screen, unsigned int pixels)
{
  unsigned int ui;
  uint32 evenval, oddval, e_r, e_g, e_b, o_r, o_g, o_b;
  uint32 outval, w1, w2;

  for (ui = 0; ui < pixels >> 1; ui++) {
    evenval = ((uint32 *)evendata)[ui]; /* two words of input data */
    oddval = ((uint32 *)odddata)[ui];   /* two words of input data */
    e_r = (evenval >> uiplot_redshift) & 0x001f001f;
    e_g = (evenval >> uiplot_greenshift) & 0x001f001f;
    e_b = (evenval >> uiplot_blueshift) & 0x001f001f;
    o_r = (oddval >> uiplot_redshift) & 0x001f001f;
    o_g = (oddval >> uiplot_greenshift) & 0x001f001f;
    o_b = (oddval >> uiplot_blueshift) & 0x001f001f;
    outval = (((e_r + o_r) >> 1) & 0x001f001f) << uiplot_redshift |
        (((e_g + o_g) >> 1) & 0x001f001f) << uiplot_greenshift |
        (((e_b + o_b) >> 1) & 0x001f001f) << uiplot_blueshift;
    w1 = (outval & 0xffff);
    w1 |= w1 << 16;
    w2 = outval & 0xffff0000;
    w2 |= w2 >> 16;
#ifdef WORDS_BIGENDIAN
    ((uint32 *)screen)[(ui << 1)] = w2;
    ((uint32 *)screen)[(ui << 1) + 1] = w1;
#else
    ((uint32 *)screen)[(ui << 1)] = w1;
    ((uint32 *)screen)[(ui << 1) + 1] = w2;
#endif
  }
}

/*** uiplot_irender32_weavefilter - take even and odd fields, filter and
     plot (32 bit) ***/

void uiplot_irender32_weavefilter(uint32 *evendata, uint32 *odddata,
                                  uint8 *screen, unsigned int pixels)
{
  unsigned int ui;
  uint32 evenval, oddval;

  for (ui = 0; ui < pixels; ui++) {
    evenval = evendata[ui];
    oddval = odddata[ui];
    /* with 32-bit data we know that there are no touching bits */
    ((uint32 *)screen)[(ui << 1) + 0] = (evenval >> 1) + (oddval >> 1);
    ((uint32 *)screen)[(ui << 1) + 1] = (evenval >> 1) + (oddval >> 1);
  }
}

