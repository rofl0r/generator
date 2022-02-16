/* ../hdr/config.h.  Generated automatically by configure.  */

/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* config.h                                                                  */
/*                                                                           */
/*****************************************************************************/

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* The number of bytes in a unsigned char.  */
#define SIZEOF_UNSIGNED_CHAR 1

/* The number of bytes in a unsigned int.  */
#define SIZEOF_UNSIGNED_INT 4

/* The number of bytes in a unsigned short.  */
#define SIZEOF_UNSIGNED_SHORT 2

/* The number of bytes in a unsigned long.  */
#define SIZEOF_UNSIGNED_LONG 4

/* The number of bytes in a unsigned long long.  */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The number of bytes in a unsigned long long long.  */
/* #undef SIZEOF_UNSIGNED_LONG_LONG_LONG */

/* Return type of the signal handler */
#define RETSIGTYPE void

/* Undefine if your processor can access 32 bits of data aligned to 16 bits */
#define ALIGNLONGS 1

/* headers */
/* #undef HAVE_TCL8_0_H */
#define HAVE_TCL_H 1
/* #undef HAVE_TK8_0_H */
#define HAVE_TK_H 1
	
/* Define your processor type if it's here */
/* #undef PROCESSOR_SPARC */
/* #undef PROCESSOR_ARM */
#define PROCESSOR_INTEL 1

/* Define if your operating system is listed */
/* #undef OS_ACORN */

/* Depending on the processor and/or C compiler the bit ordering within
   a byte sometimes gets swapped around, this should be defined if you
   use a SPARC, unset if you use an Intel */
/* #undef BYTES_HIGHFIRST */
