#ifndef DIB_HEADER_FILE
#define DIB_HEADER_FILE

/* bit map info header BITMAPINFOHEADER */

typedef struct {
  uint32 biSize;
  uint32 biWidth;
  uint32 biHeight;
  uint16 biPlanes;
  uint16 biBitCount;
  uint32 biCompression; /* a DIB can be compressed using run length encoding */
  uint32 biSizeImage;
  uint32 biXPelsPerMeter;
  uint32 biYPelsPerMeter;
  uint32 biClrUsed;
  uint32 biClrImportant;
} t_bmih;

void dib_setheader(t_bmih *bmih, uint32 x, uint32 y);

#endif /* DIB_HEADER_FILE */
