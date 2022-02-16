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
  uint32 a[2];

  if (argc < 3 ||
      sscanf(argv[1], "%d", &a[0]) == 0 ||
      sscanf(argv[2], "%d", &a[1]) == 0) {
    printf("Syntax: <dstdata> <srcdata>\n");
    exit(1);
  }

  sint32 dstdata = a[0];
  uint16 srcdata = a[1];

  uint32 outdata;
  sint32 quotient;
  sint16 remainder;

  uint8 NFLAG, VFLAG;
  uint8 CFLAG, ZFLAG;

  if (srcdata == 0) {
    printf("divide by zero\n");
    exit(0);
  }
  printf("Processing %08X / %04X\n", dstdata, srcdata);
  quotient = dstdata / (sint16)srcdata;
  remainder = dstdata % (sint16)srcdata;
  printf("C quotient = %x (%d), C remainder = %x (%d)\n", quotient,
         quotient, remainder, remainder);
  if (((quotient & 0xffff8000) == 0) ||
      ((quotient & 0xffff8000) == 0xffff8000)) {
    if ((quotient < 0) != (remainder < 0))
      remainder = -remainder;
    outdata = ((uint16)quotient) | (((uint16)(remainder))<<16);
    VFLAG = 0;
    NFLAG = ((sint16)quotient) < 0;
    ZFLAG = !((uint16)quotient);
    printf("result = %x\n", outdata);
    printf("NFLAG = %d\n", NFLAG);
    printf("VFLAG = %d\n", VFLAG);
    printf("ZFLAG = %d\n", ZFLAG);
  } else {
    VFLAG = 1;
    NFLAG = 1;
    printf("NFLAG = %d\n", NFLAG);
    printf("VFLAG = %d\n", VFLAG);
  }
  CFLAG = 0;
  printf("CFLAG = %d\n", CFLAG);
  printf("XFLAG unchanged\n");
  exit(0);
}
