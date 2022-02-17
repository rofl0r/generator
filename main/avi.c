/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"

#include "vdp.h"
#include "ui.h"
#include "dib.h"
#include "avi.h"

#ifdef JPEG
#  include "jpeglib.h"
#endif

/* From vfw.h */

/* The following is a short description of the AVI file format.  Please
 * see the accompanying documentation for a full explanation.
 *
 * An AVI file is the following RIFF form:
 *
 *  RIFF('AVI' 
 *        LIST('hdrl'
 *        avih(<MainAVIHeader>)
 *                  LIST ('strl'
 *                      strh(<Stream header>)
 *                      strf(<Stream format>)
 *                      ... additional header data
 *            LIST('movi'  
 *          { LIST('rec' 
 *                SubChunk...
 *             )
 *              | SubChunk } ....     
 *            )
 *            [ <AVIIndex> ]
 *      )
 *
 *  The main file header specifies how many streams are present.  For
 *  each one, there must be a stream header chunk and a stream format
 *  chunk, enlosed in a 'strl' LIST chunk.  The 'strf' chunk contains
 *  type-specific format information; for a video stream, this should
 *  be a BITMAPINFO structure, including palette.  For an audio stream,
 *  this should be a WAVEFORMAT (or PCMWAVEFORMAT) structure.
 *
 *  The actual data is contained in subchunks within the 'movi' LIST 
 *  chunk.  The first two characters of each data chunk are the
 *  stream number with which that data is associated.
 *
 *  Some defined chunk types:
 *           Video Streams:
 *                  ##db: RGB DIB bits
 *                  ##dc: RLE8 compressed DIB bits
 *                  ##pc: Palette Change
 *
 *           Audio Streams:
 *                  ##wb: waveform audio bytes
 *
 * The grouping into LIST 'rec' chunks implies only that the contents of
 *   the chunk should be read into memory at the same time.  This
 *   grouping is used for files specifically intended to be played from 
 *   CD-ROM.
 *
 * The index chunk at the end of the file should contain one entry for 
 *   each data chunk in the file.
 *       
 * Limitations for the current software:
 *  Only one video stream and one audio stream are allowed.
 *  The streams must start at the beginning of the file.
 *
 * 
 * To register codec types please obtain a copy of the Multimedia
 * Developer Registration Kit from:
 *
 *  Microsoft Corporation
 *  Multimedia Systems Group
 *  Product Marketing
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 */

#ifdef JPEG
struct jpeg_compress_struct avi_cinfo;
struct jpeg_error_mgr avi_jerr;
#endif

t_avi *avi_open(char *filename, t_aviinfo *info, int jpeg)
{
  t_avi *avi;
  int saved_errno;
  t_avi_header h;
  unsigned int rate;
  uint32 usperframe;
  t_bmih dib;
  t_bmih *diblittle;

  memset(&h, 0, sizeof(t_avi_header));
  if ((avi = malloc(sizeof(t_avi))) == NULL)
    return NULL;
  memset(avi, 0, sizeof(t_avi));
  avi->fd = fopen(filename, "wb");
  if (avi->fd == NULL) {
    saved_errno = errno;
    free(avi);
    errno = saved_errno;
    return NULL;
  }
  memcpy(&avi->info, info, sizeof(avi->info));

  /* record items that need to be post-written */
  avi->off_length = (char *)&h.length - (char *)&h;
  avi->off_movilength = (char *)&h.movi.length - (char *)&h;
  avi->off_totalframes = (char *)&h.hdrl.avih.TotalFrames - (char *)&h;
  avi->off_videolength = (char *)&h.hdrl.strl_video.strh.Length - (char *)&h;
  avi->off_audiolength = (char *)&h.hdrl.strl_audio.strh.Length - (char *)&h;

  rate = 0;
  rate+= info->width * info->height * 3; /* 24 bit bmp */
  rate+= 2 * 2 * info->sampspersec * info->fps / 1000; /* stereo, 16 bit */
  usperframe = (long long)1000000000 / info->fps;
  memcpy(h.riff, "RIFF", 4);
  h.length = 0; /* to be filled in later */
  memcpy(h.avi, "AVI ", 4);
  memcpy(h.hdrl.list, "LIST", 4);
  h.hdrl.length = LOCENDIAN32L(sizeof(h.hdrl) - 8);
  memcpy(h.hdrl.hdrl, "hdrl", 4);
  memcpy(h.hdrl.avih.avih, "avih", 4);
  h.hdrl.avih.length = LOCENDIAN32L(sizeof(h.hdrl.avih) - 8);
  h.hdrl.avih.MicroSecPerFrame = LOCENDIAN32L(usperframe);
  h.hdrl.avih.MaxBytesPerSec = LOCENDIAN32L(rate * info->fps / 1000);
  h.hdrl.avih.PaddingGranularity = 0;
  h.hdrl.avih.Flags = LOCENDIAN32L(AVIF_ISINTERLEAVED);
  h.hdrl.avih.TotalFrames = 0; /* to be filled in later */
  h.hdrl.avih.InitialFrames = 0;
  h.hdrl.avih.Streams = LOCENDIAN32L(2);
  h.hdrl.avih.SuggestedBufferSize = LOCENDIAN32L(rate * 2); /* 2 for luck */
  h.hdrl.avih.Width = LOCENDIAN32L(info->width);
  h.hdrl.avih.Height = LOCENDIAN32L(info->height);

  /* VIDEO strl LIST */

  memcpy(h.hdrl.strl_video.list, "LIST", 4);
  h.hdrl.strl_video.length = LOCENDIAN32L(sizeof(h.hdrl.strl_video) - 8);
  memcpy(h.hdrl.strl_video.strl, "strl", 4);
  memcpy(h.hdrl.strl_video.strh.strh, "strh", 4);
  h.hdrl.strl_video.strh.length =
    LOCENDIAN32L(sizeof(h.hdrl.strl_video.strh) - 8);
  memcpy(h.hdrl.strl_video.strh.Type, "vids", 4);
  memset(&h.hdrl.strl_video.strh.Handler, 0, 4);
  h.hdrl.strl_video.strh.Flags = LOCENDIAN32L(0);
  h.hdrl.strl_video.strh.Priority = LOCENDIAN16L(0);
  h.hdrl.strl_video.strh.Language = LOCENDIAN16L(0);
  h.hdrl.strl_video.strh.InitialFrames = LOCENDIAN32L(0);
  h.hdrl.strl_video.strh.Scale = LOCENDIAN32L(usperframe);
  h.hdrl.strl_video.strh.Rate = LOCENDIAN32L(1000000);
  h.hdrl.strl_video.strh.Start = LOCENDIAN32L(0);
  h.hdrl.strl_video.strh.Length = 0; /* to be filled in later */
  h.hdrl.strl_video.strh.SuggestedBufferSize = LOCENDIAN32L(rate * 2);
  h.hdrl.strl_video.strh.Quality = LOCENDIAN32L(0); /* this or 10,000? */
  h.hdrl.strl_video.strh.SampleSize = LOCENDIAN32L(0); /* n/a XXX: Check */
  h.hdrl.strl_video.strh.rcFrame.left = LOCENDIAN32L(0);
  h.hdrl.strl_video.strh.rcFrame.top = LOCENDIAN32L(0);
  h.hdrl.strl_video.strh.rcFrame.right = LOCENDIAN32L(info->width);
  h.hdrl.strl_video.strh.rcFrame.bottom = LOCENDIAN32L(info->height);
  memcpy(h.hdrl.strl_video.strf.strf, "strf", 4);
  h.hdrl.strl_video.strf.length =
      LOCENDIAN32L(sizeof(h.hdrl.strl_video.strf) - 8);
  dib_setheader(&dib, info->width, info->height);
  diblittle = &h.hdrl.strl_video.strf.bmih;
  diblittle->biSize = LOCENDIAN32L(dib.biSize);
  diblittle->biWidth = LOCENDIAN32L(dib.biWidth);
  diblittle->biHeight = LOCENDIAN32L(dib.biHeight);
  diblittle->biPlanes = LOCENDIAN16L(dib.biPlanes);
  diblittle->biBitCount = LOCENDIAN16L(dib.biBitCount);
  diblittle->biCompression = LOCENDIAN32L(dib.biCompression);
  diblittle->biSizeImage = LOCENDIAN32L(dib.biSizeImage);
  diblittle->biXPelsPerMeter = LOCENDIAN32L(dib.biXPelsPerMeter);
  diblittle->biYPelsPerMeter = LOCENDIAN32L(dib.biYPelsPerMeter);
  diblittle->biClrUsed = LOCENDIAN32L(dib.biClrUsed);
  diblittle->biClrImportant = LOCENDIAN32L(dib.biClrImportant);

  /* AUDIO strl LIST */

  memcpy(h.hdrl.strl_audio.list, "LIST", 4);
  h.hdrl.strl_audio.length = LOCENDIAN32L(sizeof(h.hdrl.strl_audio) - 8);
  memcpy(h.hdrl.strl_audio.strl, "strl", 4);
  memcpy(h.hdrl.strl_audio.strh.strh, "strh", 4);
  h.hdrl.strl_audio.strh.length =
    LOCENDIAN32L(sizeof(h.hdrl.strl_audio.strh) - 8);
  memcpy(h.hdrl.strl_audio.strh.Type, "auds", 4);
  memset(&h.hdrl.strl_audio.strh.Handler, 0, 4);
  h.hdrl.strl_audio.strh.Flags = LOCENDIAN32L(0);
  h.hdrl.strl_audio.strh.Priority = LOCENDIAN16L(0);
  h.hdrl.strl_audio.strh.Language = LOCENDIAN16L(0);
  h.hdrl.strl_audio.strh.InitialFrames = LOCENDIAN32L(0);
  h.hdrl.strl_audio.strh.Scale = LOCENDIAN32L(1);
  h.hdrl.strl_audio.strh.Rate = LOCENDIAN32L(info->sampspersec);
  h.hdrl.strl_audio.strh.Start = LOCENDIAN32L(0);
  h.hdrl.strl_audio.strh.Length = 0; /* to be filled in later */
  h.hdrl.strl_audio.strh.SuggestedBufferSize = 0; /* XXX: ok? */
  h.hdrl.strl_audio.strh.Quality = LOCENDIAN32L(0); /* this or 10,000? */
  h.hdrl.strl_audio.strh.SampleSize = LOCENDIAN32L(4); /* stereo 16 bit */
  h.hdrl.strl_audio.strh.rcFrame.left = LOCENDIAN32L(0); /* n/a */
  h.hdrl.strl_audio.strh.rcFrame.top = LOCENDIAN32L(0); /* n/a */
  h.hdrl.strl_audio.strh.rcFrame.right = LOCENDIAN32L(0); /* n/a */
  h.hdrl.strl_audio.strh.rcFrame.bottom = LOCENDIAN32L(0); /* n/a */
  memcpy(h.hdrl.strl_audio.strf.strf, "strf", 4);
  h.hdrl.strl_audio.strf.length =
      LOCENDIAN32L(sizeof(h.hdrl.strl_audio.strf) - 8);
  h.hdrl.strl_audio.strf.wFormatTag = LOCENDIAN16L(WAVE_FORMAT_PCM);
  h.hdrl.strl_audio.strf.nChannels = LOCENDIAN16L(2);
  h.hdrl.strl_audio.strf.nSamplesPerSec = LOCENDIAN32L(info->sampspersec);
  h.hdrl.strl_audio.strf.nAvgBytesPerSec = LOCENDIAN32L(info->sampspersec * 4);
  h.hdrl.strl_audio.strf.nBlockAlign = LOCENDIAN16L(4); /* stereo 16 bit */
  h.hdrl.strl_audio.strf.wBitsPerSample = LOCENDIAN16L(16);

  /* trailing movi... */

  memcpy(h.movi.list, "LIST", 4);
  h.movi.length = 0; /* to be filled in later */
  memcpy(h.movi.movi, "movi", 4);

#ifdef JPEG
  if (jpeg) {
    memcpy(&h.hdrl.strl_video.strh.Handler, "MJPG", 4);
    memcpy(&diblittle->biCompression, "MJPG", 4);
    avi->jpeg = 1;
    avi->bgr = 0; /* jpeg needs RGB data */
    avi_cinfo.err = jpeg_std_error(&avi_jerr);
    jpeg_create_compress(&avi_cinfo);
    jpeg_stdio_dest(&avi_cinfo, avi->fd);
    avi_cinfo.image_width = avi->info.width;
    avi_cinfo.image_height = avi->info.height;
    avi_cinfo.input_components = 3;
    avi_cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&avi_cinfo);
    jpeg_set_quality(&avi_cinfo, info->jpegquality, 0);
  } else {
#endif
    (void) jpeg;
    avi->jpeg = 0;
    avi->bgr = 1; /* uncompressed avi needs BGR data */
#ifdef JPEG
  }
#endif

  /* pre-create ##db, ##wb chunk headers */
  strncpy(avi->dibchunkh, "00db", sizeof avi->dibchunkh);
  strncpy(avi->wavechunkh, "01wb", sizeof avi->wavechunkh);

  avi->linebytes = ((3 * info->width) + 3) & ~3; /* round up */
  if (fwrite(&h, sizeof(h), 1, avi->fd) != 1) {
    free(avi);
    fclose(avi->fd);
    errno = EIO;
    return NULL;
  }
  avi->frames = 0;
  avi->audiolength = 0;
  avi->movilength = 4; /* "movi" string */
  return avi;
}

static int avi_update32(t_avi *avi, uint32 offset, uint32 data)
{
  uint32 little_data;

  if (fseek(avi->fd, offset, SEEK_SET) != 0)
    return -1;

  little_data = LOCENDIAN32L(data);
  if (fwrite(&little_data, 4, 1, avi->fd) != 1) {
    errno = EIO;
    return -1;
  }
  return 0;
}

int avi_close(t_avi *avi)
{
  int saved_errno;
  long pos;

  if ((pos = ftell(avi->fd)) == -1)
    return -1;

  if (avi_update32(avi, avi->off_length, pos - 8) != 0 ||
      avi_update32(avi, avi->off_movilength, avi->movilength) != 0 ||
      avi_update32(avi, avi->off_totalframes, avi->frames) != 0 ||
      avi_update32(avi, avi->off_videolength, avi->frames) != 0 ||
      avi_update32(avi, avi->off_audiolength, avi->audiolength) != 0) {
    saved_errno = errno;
    fclose(avi->fd);
    errno = saved_errno;
    return -1;
  }
  if (fclose(avi->fd) != 0)
    return -1;
#ifdef JPEG
  jpeg_destroy_compress(&avi_cinfo);
#endif
  return 0;
}

#ifdef JPEG

int avi_video_jpeg(t_avi *avi, uint8 *video)
{
  JSAMPROW row[1];
  unsigned int line;
  long pos1, pos2;
  int pad;
  uint32 zero = 0;

  if ((pos1 = ftell(avi->fd)) == -1)
    return -1;

  *(uint32 *)(avi->dibchunkh + 4) = 0;
  if (fwrite(avi->dibchunkh, 8, 1, avi->fd) != 1) {
    errno = EIO;
    return -1;
  }
  jpeg_start_compress(&avi_cinfo, TRUE);
  for (line = 0; line < avi->info.height; line++) {
    row[0] = video + line * (3 * avi->info.width);
    jpeg_write_scanlines(&avi_cinfo, row, 1);
  }
  jpeg_finish_compress(&avi_cinfo);

  if ((pos2 = ftell(avi->fd)) == -1)
    return -1;

  pad = ((pos2 & 3) == 0 ? 0 : (4 - (pos2 & 3)));
  if (fwrite(&zero, pad, 1, avi->fd) != 1) {
    errno = EIO;
    return -1;
  }

  /* write chunk length at pos1 + 4 */
  avi_update32(avi, pos1 + 4, pos2 - pos1 + pad - 8);

  if (fseek(avi->fd, 0, SEEK_END) != 0)
    return -1;

  avi->frames+= 1;
  avi->movilength+= pos2 - pos1 + pad;
  return 0;
}

#endif

int avi_video_raw(t_avi *avi, uint8 *video)
{
  uint32 size;
  unsigned int line, x;
  uint8 buf[3 * 1024];
  uint8 *p, *v;

  if (avi->linebytes > sizeof(buf))
    ui_err("avi_video_raw buffer too small");

  size = avi->linebytes * avi->info.height;

  *(uint32 *)(avi->dibchunkh + 4) = LOCENDIAN32L(size);
  if (fwrite(avi->dibchunkh, 8, 1, avi->fd) != 1) {
    errno = EIO;
    return -1;
  }
  /* convert from RGB to BGR */
  for (line = 0; line < avi->info.height; line++) {
    v = video + (avi->info.height - line - 1) * (avi->info.width * 3);
    p = buf;
    for (x = 0; x < avi->info.width; x++) {
      *p++ = v[2];
      *p++ = v[1];
      *p++ = v[0];
      v+= 3;
    }
    /* we should have avi->linebytes of data - pad with zeros */
    for (x = 0; x < (avi->linebytes - (avi->info.width * 3)); x++)
      *p++ = '\0'; /* pad */
    if (fwrite(buf, avi->linebytes, 1, avi->fd) != 1) {
      errno = EIO;
      return -1;
    }
  }
  avi->frames+= 1;
  avi->movilength+= 8 + avi->linebytes * avi->info.height;
  return 0;
}

int avi_video(t_avi *avi, uint8 *video)
{
#ifdef JPEG
  if (avi->jpeg)
    return avi_video_jpeg(avi, video);
#endif
  return avi_video_raw(avi, video);
}

int avi_audio(t_avi *avi, uint8 *audio, uint32 samples)
{
  /* audio format is left/right 16 bit little samples */

  *(uint32 *)(avi->wavechunkh + 4) = LOCENDIAN32L(4 * samples);
  if (fwrite(avi->wavechunkh, 8, 1, avi->fd) != 1 ||
      fwrite(audio, 4 * samples, 1, avi->fd) != 1) {
    errno = EIO;
    return -1;
  }
  avi->audiolength+= samples;
  avi->movilength+= 8 + 4 * samples;
  return 0;
}

