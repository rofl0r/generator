/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* plotter routines for user interfaces - used by console, gtk, sdl */

#include "generator.h"
#include "vdp.h"

#include "uiplot.h"

uint32 uiplot_palcache[192];
uint32 uiplot_palcache_yuv[192];

static uint32 uiplot_yuvcache[512];

static int uiplot_redshift;
static int uiplot_greenshift;
static int uiplot_blueshift;

static uint32 rgb2yuv(uint32 r, uint32 g, uint32 b)
{
  uint32 y, u, v;

  y = (( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16) & 0xff;
  u = (( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128) & 0xff;
  v = (( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128) & 0xff;

  return (v << 16) | (u << 8) | y;
}

void uiplot_setshifts(int redshift, int greenshift, int blueshift)
{
  unsigned int r, g, b;

  uiplot_redshift = redshift;
  uiplot_greenshift = greenshift;
  uiplot_blueshift = blueshift;

  for (b = 0; b < 8; b++)
    for (g = 0; g < 8; g++)
      for (r = 0; r < 8; r++)
        uiplot_yuvcache[(r << 6) | (g << 3) | b] =
		rgb2yuv(r << 5, g << 5, b << 5);
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
    uint32 r, g, b, v;

    if (!flag && !vdp_cramf[col])
      continue;
    vdp_cramf[col] = 0;
    p = (uint8 *)vdp_cram + 2 * col;  /* point p to the two-byte CRAM entry */
    r = (p[1] & 0xE) << 3;
    g = (p[1] & 0xE0) >> 1;
    b = (p[0] & 0xE) << 3;

    uiplot_palcache_yuv[col] = rgb2yuv(r << 1, g << 1, b << 1);
    uiplot_palcache_yuv[col + 64] = rgb2yuv(r | 128, g | 128, b | 128);
    uiplot_palcache_yuv[col + 128] = rgb2yuv(r, g, b);

    r = (p[1] & 0xE) << uiplot_redshift;
    g = ((p[1] & 0xE0) >> 4) << uiplot_greenshift;
    b = (p[0] & 0xE) << uiplot_blueshift;

    uiplot_palcache[col] = r << 1 | g << 1 | b << 1; /* normal */
    v = r | g | b;
    uiplot_palcache[col + 64] = v | /* hilight */
      16 << uiplot_redshift |
      16 << uiplot_greenshift |
      16 << uiplot_blueshift;
    uiplot_palcache[col + 128] = v;   /* shadow */

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
  pixels >>= 2;
  for (ui = 0; ui < pixels; ui++) {
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

/*
#define UNROLL(t, code) 			\
do { 						\
if ((t) > 0) { code }				\
if ((t) > 1) { code }				\
if ((t) > 2) { code }				\
if ((t) > 3) { code }				\
if ((t) > 4) { code }				\
if ((t) > 5) { code }				\
if ((t) > 6) { code }				\
if ((t) > 7) { code }				\
} while (0)
*/

#define UNROLL(t, code)	\
do {			\
  switch (t) {		\
  case 16: { code }	\
  case 15: { code }	\
  case 14: { code }	\
  case 13: { code }	\
  case 12: { code }	\
  case 11: { code }	\
  case 10: { code }	\
  case  9: { code }	\
  case  8: { code }	\
  case  7: { code }	\
  case  6: { code }	\
  case  5: { code }	\
  case  4: { code }	\
  case  3: { code }	\
  case  2: { code }	\
  case  1: { code }	\
  }			\
} while(0)

/* Valid values: 1, 2, 4 */
#define UNROLL_TIMES 4 

/* Prefetching is only effective if supported by architecture i.e.,
 * you must use -march (e.g., -march=athlon)!  
 */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && \
	(__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))

#define PREFETCH_R(x) __builtin_prefetch((x), 0)
#define PREFETCH_W(x) __builtin_prefetch((x), 1)

#else /* !(GCC >= 3.1) */

#define PREFETCH_R(x)
#define PREFETCH_W(x)
#endif /* GCC >= 3.1 */

void uiplot_convertdata_yuy2(uint8 *indata, uint16 *out, unsigned int pixels)
{
  unsigned int i;
  uint32 a, b, uy, *out32 = (uint32 *) out;

#ifdef WORDS_BIGENDIAN
  uint8 u, v;
#define PACK_PIXELS()	do {						\
    u = ((a & 0xff00) + (b & 0xff00)) >> 9;				\
    *out++ = (a << 8) | u;						\
    v = ((a & 0xff0000) + (b & 0xff0000)) >> 17;			\
    *out++ = (b << 8) | v;						\
} while (0)
#else /* !WORDS_BIGENDIAN */
  uint32 v;
#define PACK_PIXELS()	do {						\
    v = (((a + b) & 0x1fe0000) << 7) | (a & 0xff) | ((b & 0xff) << 16);	\
    uy = ((((a & 0xff00) + (b & 0xff00)) >> 1) & 0xff00);		\
    *out32++ = v | uy;							\
} while (0)
#endif /* WORDS_BIGENDIAN */

  /* not scaled, 16bpp - we do 2 pixels at a time */
  PREFETCH_W(out);
  for (i = pixels; i != 0; i -= 2 * UNROLL_TIMES) {
    UNROLL(UNROLL_TIMES,
    	a = uiplot_palcache_yuv[*indata++];
    	b = uiplot_palcache_yuv[*indata++];
	PACK_PIXELS();
    ); /* UNROLL */
    PREFETCH_R(indata + 32);
  }

#undef PACK_PIXELS
}

void uiplot_convertdata_uyvy(uint8 *indata, uint16 *out, unsigned int pixels)
{
  uint32 a, b;
  unsigned int i;
  uint32 vy, uy, *out32 = (uint32 *)out;

#ifdef WORDS_BIGENDIAN
  uint32 u, v;
#define PACK_PIXELS()	do {						\
    u = ((a & 0xff00) + (b & 0xff00)) >> 1;				\
    *out++ = (a & 0xff) | (u & 0xff00);					\
    v = ((a & 0xff0000) + (b & 0xff0000)) >> 9;				\
    *out++ = (b & 0xff) | (v & 0xff00);					\
} while (0)
#else
#define PACK_PIXELS()	do {						\
    uy = (((a & 0xff00) + (b & 0xff00)) >> 9) | ((a & 0xff) << 8);	\
    vy = (((a + b) >> 1) & 0xff0000) | (b << 24);			\
    *out32++ = uy | vy;							\
} while (0)
#endif

  PREFETCH_W(out);
  /* not scaled, 16bpp - we do 2 pixels at a time */
  for (i = pixels; i != 0; i -= 2 * UNROLL_TIMES) {
    UNROLL(UNROLL_TIMES,
    	a = uiplot_palcache_yuv[*indata++];
   	b = uiplot_palcache_yuv[*indata++];
	PACK_PIXELS();
    ); /* UNROLL */
   PREFETCH_R(indata + 32);
  }

#undef PACK_PIXELS
}

void uiplot_convertdata_yvyu(uint8 *indata, uint16 *out, unsigned int pixels)
{
  unsigned int i;
  uint32 a, b, yu, yv, *out32 = (uint32 *) out;

#ifdef WORDS_BIGENDIAN
  uint8 u, v;

#define PACK_PIXELS()	do {						\
    v = ((a & 0xff0000) + (b & 0xff0000)) >> 17;			\
    *out++ = (a << 8) | v;						\
    u = ((a & 0xff00) + (b & 0xff00)) >> 9;				\
    *out++ = (b << 8) | u;						\
} while (0)
#else /* !WORDS_BIGENDIAN */
#define PACK_PIXELS()	do {						 \
    yu = (((a + b) & 0x1fe0000) << 7) | (a & 0xff) | ((b & 0xff) << 16); \
    yv = ((((a & 0xff00) + (b & 0xff00)) >> 1) & 0xff00);		 \
    *out32++ = yv | yu;							 \
} while (0)
#endif /* WORDS_BIGENDIAN */

  /* not scaled, 16bpp - we do 2 pixels at a time */
  PREFETCH_W(out);
  for (i = pixels; i != 0; i -= 2 * UNROLL_TIMES) {
    UNROLL(UNROLL_TIMES,
    	a = uiplot_palcache_yuv[*indata++];
    	b = uiplot_palcache_yuv[*indata++];
	PACK_PIXELS();
    ); /* UNROLL */
    PREFETCH_R(indata + 32);
  }

#undef PACK_PIXELS
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
  pixels >>= 2;
  for (ui = 0; ui < pixels; ui++) {
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

  pixels >>= 1;
  for (ui = 0; ui < pixels; ui++) {
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
  pixels >>= 1;
  for (ui = 0; ui < pixels; ui++) {
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
  pixels >>= 1;
  for (ui = 0; ui < pixels; ui++) {
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

  pixels >>= 1;
  for (ui = 0; ui < pixels; ui++) {
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

