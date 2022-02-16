#include <stdio.h>

/* SYNTAX: a.out A B Z */
/* e.g. ./a.out 31 32 1 subs 0x31 - 0x32 with Z flag set */

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;

int main(int argc, char *argv[])
{
  uint16 a[3];

  if (argc < 3 ||
      sscanf(argv[1], "%x", &a[0]) == 0 ||
      sscanf(argv[2], "%x", &a[1]) == 0 ||
      sscanf(argv[3], "%x", &a[2]) == 0) {
    printf("Syntax: <hexval> <hexval> <xflag>\n");
    exit(1);
  }

  uint8 dstdata = a[0];
  uint8 srcdata = a[1];
  uint8 XFLAG = a[2];
  uint8 NFLAG, VFLAG;
  uint8 CFLAG;

  uint8 outdata;
  sint8 outdata_low = (dstdata & 0xF) - (srcdata & 0xF) - XFLAG;
  sint16 precalc = dstdata - srcdata - XFLAG;
  sint16 outdata_tmp = precalc;

  if (outdata_low < 0)
    outdata_tmp-= 0x06;
  if (outdata_tmp < 0) {
    outdata_tmp-= 0x60;
    CFLAG = 1;
    XFLAG = 1;
  } else {
    CFLAG = 0;
    XFLAG = 0;
  }
  outdata = outdata_tmp;
  if (outdata) printf("ZFLAG = 0\n");
  NFLAG = ((sint8)outdata) < 0;
  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);
  printf("CFLAG = %d\n", CFLAG);
  printf("XFLAG = %d\n", XFLAG);
  printf("NFLAG = %d\n", NFLAG);
  printf("VFLAG = %d\n", VFLAG);
  printf("( precalc = %X outdata = %X )\n", precalc, outdata);
  printf("%X - %X (- %X) = %X\n", dstdata, srcdata, XFLAG, outdata);
  exit(0);
}
