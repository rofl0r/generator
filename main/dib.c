/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"

#include "dib.h"

/* From http://www.jmcgowan.com/avitech.html */

/*
 * The DIB files have a standard header that identifies the format, size,
 * color palette (if applicable) of the bitmapped image.  The header
 * is a BITMAPINFO structure.
 * 
 * typedef struct tagBITMAPINFO {
 *     BITMAPINFOHEADER    bmiHeader;
 *     RGBQUAD             bmiColors[1];
 * } BITMAPINFO;
 * 
 * The BITMAPINFOHEADER is a structure of the form:
 * 
 * typedef struct tagBITMAPINFOHEADER{ // bmih 
 *    DWORD  biSize; 
 *    LONG   biWidth; 
 *    LONG   biHeight; 
 *    WORD   biPlanes; 
 *    WORD   biBitCount 
 *    DWORD  biCompression; // a DIB can be compressed using run length encoding
 *    DWORD  biSizeImage; 
 *    LONG   biXPelsPerMeter; 
 *    LONG   biYPelsPerMeter; 
 *    DWORD  biClrUsed; 
 *    DWORD  biClrImportant; 
 * } BITMAPINFOHEADER; 
 *  
 * bmiColors[1] is the first entry in an optional color palette or color
 * table of RGBQUAD data structures.  True color (24 bit RGB) images
 * do not need a color table.  4 and 8 bit color images use a color table.
 * 
 * typedef struct tagRGBQUAD { // rgbq 
 *     BYTE    rgbBlue; 
 *     BYTE    rgbGreen; 
 *     BYTE    rgbRed; 
 *     BYTE    rgbReserved; // always zero
 * } RGBQUAD; 
 *  
 * A DIB consists of
 * 
 *   (BITMAPINFOHEADER)(optional color table of RGBQUAD's)(data for the
 * bitmapped image)
 * 
 */

/*
 * Other useful links:
 * http://www.cwi.nl/ftp/audio/RIFF-format
 *
 */

void dib_setheader(t_bmih *bmih, uint32 x, uint32 y)
{
  uint8 pad = 4 - ((x * 3) & 3); /* must align to 32 bits (4 bytes) */

  if (pad == 4)
    pad = 0; 
  bmih->biSize = sizeof(t_bmih);
  bmih->biWidth = x;
  bmih->biHeight = y;
  bmih->biPlanes = 1;
  bmih->biBitCount = 24;
  bmih->biCompression = 0; /* BI_RGB */
  bmih->biSizeImage = ((x * 3) + pad) * y;
  bmih->biXPelsPerMeter = 0;
  bmih->biYPelsPerMeter = 0;
  bmih->biClrUsed = 0; /* no palette in 24 bit mode */
  bmih->biClrImportant = 0; /* no palette in 24 bit mode */
  assert(bmih->biSize == 40);
}
