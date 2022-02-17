#ifndef AVI_HEADER_FILE
#define AVI_HEADER_FILE

/* these structures are based on vfw.h
 *
 * here is a nice example from http://www.microsoft.com/
 *   DirectX/dxm/help/ds/FiltDev/DV_Data_AVI_File_Format.htm
 * 
 * 00000000 RIFF (103E2920) 'AVI '
 * 0000000C     LIST (00000146) 'hdrl'
 * 00000018         avih (00000038)
 *                      dwMicroSecPerFrame    : 33367
 *                      dwMaxBytesPerSec      : 3728000
 *                      dwPaddingGranularity  : 0
 *                      dwFlags               : 0x810 HASINDEX | TRUSTCKTYPE
 *                      dwTotalFrames         : 2192
 *                      dwInitialFrames       : 0
 *                      dwStreams             : 2
 *                      dwSuggestedBufferSize : 120000
 *                      dwWidth               : 720
 *                      dwHeight              : 480
 *                      dwReserved            : 0x0
 * 00000058         LIST (00000094) 'strl'
 * 00000064             strh (00000038)
 *                          fccType               : 'vids'
 *                          fccHandler            : 'dvsd'
 *                          dwFlags               : 0x0
 *                          wPriority             : 0
 *                          wLanguage             : 0x0 undefined
 *                          dwInitialFrames       : 0
 *                          dwScale               : 100 (29.970 Frames/Sec)
 *                          dwRate                : 2997
 *                          dwStart               : 0
 *                          dwLength              : 2192
 *                          dwSuggestedBufferSize : 120000
 *                          dwQuality             : 0
 *                          dwSampleSize          : 0
 *                          rcFrame               : 0,0,720,480
 * 000000A4             strf (00000048)
 *                          biSize          : 40
 *                          biWidth         : 720
 *                          biHeight        : 480
 *                          biPlanes        : 1
 *                          biBitCount      : 24
 *                          biCompression   : 0x64737664 'dvsd'
 *                          biSizeImage     : 120000
 *                          biXPelsPerMeter : 0
 *                          biYPelsPerMeter : 0
 *                          biClrUsed       : 0
 *                          biClrImportant  : 0
 *                          dwDVAAuxSrc     : 0x........
 *                          dwDVAAuxCtl     : 0x........
 *                          dwDVAAuxSrc1    : 0x........
 *                          dwDVAAuxCtl1    : 0x........
 *                          dwDVVAuxSrc     : 0x........
 *                          dwDVVAuxCtl     : 0x........
 *                          dwDVReserved[2] : 0,0
 * 000000F4         LIST (0000005E) 'strl'
 * 00000100             strh (00000038)
 *                          fccType               : 'auds'
 *                          fccHandler            : '    '
 *                          dwFlags               : 0x0
 *                          wPriority             : 0
 *                          wLanguage             : 0x0 undefined
 *                          dwInitialFrames       : 0
 *                          dwScale               : 1 (32000.000 Samples/Sec)
 *                          dwRate                : 32000
 *                          dwStart               : 0
 *                          dwLength              : 2340474
 *                          dwSuggestedBufferSize : 4272
 *                          dwQuality             : 0
 *                          dwSampleSize          : 4
 *                          rcFrame               : 0,0,0,0
 * 00000140             strf (00000012)
 *                          wFormatTag      : 1 PCM
 *                          nChannels       : 2
 *                          nSamplesPerSec  : 32000
 *                          nAvgBytesPerSec : 128000
 *                          nBlockAlign     : 4
 *                          wBitsPerSample  : 16
 *                          cbSize          : 0
 * 00000814     LIST (103D0EF4) 'movi'
 * 103D1710     idx1 (00011210)
 *
 */

typedef struct {
  uint32 left;
  uint32 top;
  uint32 right;
  uint32 bottom;
} t_win_rect;

typedef struct {
  char avih[4];                 /* "avih" */
  uint32 length;                /* chunk length */
  uint32 MicroSecPerFrame;      /* frame display rate (or 0L) */
  uint32 MaxBytesPerSec;        /* max. transfer rate */
  uint32 PaddingGranularity;    /* pad to multiples of this
                                   size; normally 2K. */
  uint32 Flags;                 /* the ever-present flags */
  uint32 TotalFrames;           /* # frames in file */
  uint32 InitialFrames;
  uint32 Streams;
  uint32 SuggestedBufferSize;
  uint32 Width;
  uint32 Height;
  uint32 Reserved[4];
} t_avichunk_avih;

typedef struct {
  char strh[4];                 /* "strh" */
  uint32 length;                /* chunk length */
  char Type[4];
  char Handler[4];
  uint32 Flags;                 /* Contains AVITF_* flags */
  uint16 Priority;
  uint16 Language;
  uint32 InitialFrames;
  uint32 Scale;
  uint32 Rate;                  /* dwRate / dwScale == samples/second */
  uint32 Start;
  uint32 Length;                /* In units above... */
  uint32 SuggestedBufferSize;
  uint32 Quality;
  uint32 SampleSize;
  t_win_rect rcFrame; /* should this be 4 16 bit vals or 4 32 bit vals? */
} t_avichunk_strh;

/* BITMAPINFOHEADER - see dib.h */

/* the structure below is for PCMWAVEFORMAT - as far as I understand it the
   WAVEFORMAT structure was extended to include wBitsPerSample.  The
   PCMWAVEFORMAT was in turn extended to be the WAVEFORMATEX structure which
   includes an extra word cbSize that indicates the size of the extension
   words for whatever wFormatTag specifies.  We do not include the cbSize
   word (i.e. this really is a PCMWAVEFORMAT not a WAVEFORMATEX) as this
   would un-align 32-bit words.  Some chips, Sparc for example, can't cope
   with non-aligned 32-bit access, and I want to construct this stuff in
   memory without hassles. */

/* http://www.xiph.org/archives/vorbis-dev/200108/0199.html */

typedef struct {
  char strf[4];                 /* "strf" */
  uint32 length;                /* chunk length */
  uint16 wFormatTag;            /* WAVE_FORMAT_PCM = 1 */
  uint16 nChannels;             /* 1 for mono, 2 for stereo */
  uint32 nSamplesPerSec;        /* typically 22050 or 44100 */
  uint32 nAvgBytesPerSec;       /* must be nBlockAlign * nSamplesPerSec */
  uint16 nBlockAlign;           /* must be nChannels * BitsPerSample / 8 */
  uint16 wBitsPerSample;        /* must be 8 or 16 */
} t_avichunk_strf_audio;

typedef struct {
  char strf[4];                 /* "strf" */
  uint32 length;                /* chunk length */
  t_bmih bmih;                  /* BMP header */
} t_avichunk_strf_video;

/* the structure below is a BITMAPINFO (or so says vfw.h), which is simply
   a DIB header (the structure found in BMP files) */

typedef struct {
  t_bmih bmih;                  /* DIB bit map header */
} t_avilist_strf_video;

typedef struct {
  char list[4];                 /* "LIST" */
  uint32 length;                /* list length */
  char strl[4];                 /* "strl" */
  t_avichunk_strh strh;         /* stream header block */
  t_avichunk_strf_video strf;   /* stream format block */
} t_avilist_strl_video;

typedef struct {
  char list[4];                 /* "LIST" */
  uint32 length;                /* list length */
  char strl[4];                 /* "strl" */
  t_avichunk_strh strh;         /* stream header block */
  t_avichunk_strf_audio strf;   /* stream format block */
} t_avilist_strl_audio;

typedef struct {
  char list[4];                 /* "LIST" */
  uint32 length;                /* list length */
  char hdrl[4];                 /* hdrl */
  t_avichunk_avih avih;         /* avi header */
  t_avilist_strl_video strl_video;      /* strl block for video stream */
  t_avilist_strl_audio strl_audio;      /* strl block for audio stream */
} t_avilist_hdrl;

typedef struct {
  char list[4];                 /* "LIST" */
  uint32 length;                /* list length */
  char movi[4];                 /* movi */
  /* the rest of the data is streamed to disk... */
} t_avilist_movi;

typedef struct {
  char riff[4];                 /* "RIFF" */
  uint32 length;                /* size of whole file */
  char avi[4];                  /* "AVI " */
  t_avilist_hdrl hdrl;
  t_avilist_movi movi;
} t_avi_header;

typedef struct {
  uint32 width;
  uint32 height;
  uint32 sampspersec;
  uint32 fps;                   /* fps * 1000 */
  uint32 jpegquality;           /* 0 - 100, if applicable */
} t_aviinfo;

typedef struct {
  t_aviinfo info;
  FILE *fd;
  uint32 linebytes;             /* number of video bytes per line */
  char dibchunkh[8];            /* 00dbXXXX where XXXX is size */
  char wavechunkh[8];           /* 01wbXXXX where XXXX is size */
  uint32 frames;                /* video frames done */
  uint32 audiolength;           /* samples output so far */
  uint32 movilength;            /* movi bytes output so far */
  uint32 off_length;            /* offset in file to update */
  uint32 off_movilength;        /* offset in file to update */
  uint32 off_totalframes;       /* offset in file to update */
  uint32 off_videolength;       /* offset in file to update */
  uint32 off_audiolength;       /* offset in file to update */
  uint32 error;                 /* 1000ths of a byte we haven't output */
  uint32 jpeg;                  /* writing jpegs? */
  uint32 bgr;                   /* need BGR data instead of RGB? */
} t_avi;

#define AVIF_HASINDEX       0x00000010 /* Index at end of file? */
#define AVIF_MUSTUSEINDEX   0x00000020
#define AVIF_ISINTERLEAVED  0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800 /* Use CKType to find key frames? */
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED    0x00020000
#define WAVE_FORMAT_PCM 1

t_avi *avi_open(char *filename, t_aviinfo *info, int jpeg);
int avi_close(t_avi *avi);
int avi_video(t_avi *avi, uint8 *video);
int avi_audio(t_avi *avi, uint8 *audio, uint32 samples);

#endif /* AVI_HEADER_FILE */
