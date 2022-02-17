/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"

#include "ui.h"
#include "memz80.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "cpuz80.h"
#include "vdp.h"
#include "gensound.h"

#ifdef ALLEGRO
#include "allegro.h"
#endif

#if defined(USE_BZIP2) && defined(HAVE_BZLIB_H)
#include <bzlib.h>
#endif /* USE_BZIP2 && HAVE_BZLIB_H */

#if defined(USE_ZLIB) && defined(HAVE_ZLIB_H)
#include <zlib.h>
#endif /* USE_ZLIB && HAVE_ZLIB_H */


/* Minimum and maximum ROM sizes considered valid for compressed files */
#define MIN_ROM_SIZE (100 * 1024) /* 100 KiB */
#define MAX_ROM_SIZE (32 * 1024 * 1024) /* 32 MiB */

/*** variables externed in generator.h ***/

unsigned int gen_quit = 0;
unsigned int gen_debugmode = 0;
unsigned int gen_autodetect = 1; /* 0 = no, 1 = yes */
unsigned int gen_musiclog = 0; /* 0 = no, 1 = GYM, 2 = GNM */
unsigned int gen_modifiedrom = 0; /* 0 = no, 1 = yes */
int gen_loglevel = 1;  /* 2 = NORMAL, 1 = CRITICAL */
t_cartinfo gen_cartinfo;
char gen_leafname[128];

static int gen_freerom = 0;

/*** forward references ***/

void gen_nicetext(char *out, char *in, unsigned int size);
uint16 gen_checksum(uint8 *start, unsigned int length);
void gen_setupcartinfo(void);

/*** Signal handler ***/

RETSIGTYPE gen_sighandler(int signum)
{
  if (gen_debugmode) {
    if (signum == SIGINT) {
      if (gen_quit) {
        LOG_CRITICAL(("Bye!"));
        ui_final();
        ui_err("Exiting");
      } else {
        LOG_REQUEST(("Ping - current PC = 0x%X", regs.pc));
      }
      gen_quit = 1;
    }
  } else {
    ui_final();
    exit(0);
  }
  signal(signum, gen_sighandler);
}

static const char thing[] =
	"\n\nIt's the year 2005 is there anyone out there?\n"
	"\nIf you're from another planet put your hand in the "
	"air, say yeah!\n\n";

/*** Program entry point ***/

int main(int argc, const char *argv[])
{
  int retval;
  t_sr test;

  test.sr_int = 0;
  test.sr_struct.c = 1;
  if (test.sr_int != 1) {
    fprintf(stderr,
       "%s: compilation variable WORDS_BIGENDIAN not set correctly\n",
       argv[0]);
    return 1;
  }

  if (!argc) {
    fputs(thing, stderr);
  }

  /* initialise user interface */
  if ((retval = ui_init(argc, argv)))
    return retval;

  /* initialise 68k memory system */
  if ((retval = mem68k_init()))
    ui_err("Failed to initialise mem68k module (%d)", retval);

  /* initialise z80 memory system */
  if ((retval = memz80_init()))
    ui_err("Failed to initialise memz80 module (%d)", retval);

  /* initialise vdp system */
  if ((retval = vdp_init()))
    ui_err("Failed to initialise vdp module (%d)", retval);

  /* initialise cpu system */
  if ((retval = cpu68k_init()))
    ui_err("Failed to initialise cpu68k module (%d)", retval);

  /* initialise z80 cpu system */
  if ((retval = cpuz80_init()))
    ui_err("Failed to initialise cpuz80 module (%d)", retval);

  /* initialise sound system */
  if ((retval = sound_init()))
    ui_err("Failed to initialise sound module (%d)", retval);

  signal(SIGINT, gen_sighandler);
  signal(SIGPIPE, SIG_IGN);

  /* enter user interface loop */
  return ui_loop();
}

#ifdef ALLEGRO
END_OF_MAIN();
#endif

/*** gen_reset - reset system ***/

void gen_reset(void)
{
  vdp_reset();
  cpu68k_reset();
  cpu68k_ram_clear();
  cpuz80_reset();
  if (sound_reset()) {
    ui_err("sound failure");
  }
}

/*** gen_softreset - reset system ***/

void gen_softreset(void)
{
  cpu68k_reset();
}

#define REALLOC_THRESH (256 * 1024)
#define REALLOC_INCR (REALLOC_THRESH * 4)

#ifdef USE_BZIP2
static char *loadimage_bzip2(FILE *f)
{
  BZFILE *bzf;
  int bzerror;
  void *newbuf = NULL;
  size_t len = 0, size = 0;
  int err;
  char *error;

#define FAIL(x) do { error = (x); goto failure; } while (0)

  cpu68k_rom = NULL;
  cpu68k_romlen = 0;

  bzf = BZ2_bzReadOpen(&bzerror, f, 0, 1, NULL, 0);
  if (bzerror != BZ_OK)
    return ("BZ2_bzReadOpen failed!");

  do {
    if (size - len < REALLOC_THRESH) {
      if (size > MAX_ROM_SIZE)
        FAIL("ROM is too large");

      if (NULL == (newbuf = realloc(cpu68k_rom, size + REALLOC_INCR)))
        FAIL("Out of memory!");

      cpu68k_rom = newbuf;
      memset(cpu68k_rom + size, 0, REALLOC_INCR);
      size += REALLOC_INCR;
    }

    len += BZ2_bzRead(&bzerror, bzf, cpu68k_rom + len, REALLOC_THRESH);
  } while (bzerror == BZ_OK);

  err = bzerror;
  BZ2_bzReadClose(&bzerror, bzf);
  bzf = NULL;

  if (err != BZ_STREAM_END) {
    switch (err) {
    case BZ_PARAM_ERROR:	FAIL("BZ2_bzRead(): Parameter error");
    case BZ_SEQUENCE_ERROR:	FAIL("BZ2_bzRead(): Sequence error");
    case BZ_IO_ERROR:       	FAIL("BZ2_bzRead(): I/O error");
    case BZ_UNEXPECTED_EOF: 	FAIL("BZ2_bzRead(): Unexpected EOF");
    case BZ_DATA_ERROR:     	FAIL("BZ2_bzRead(): Data integrity error");
    case BZ_DATA_ERROR_MAGIC:   FAIL("BZ2_bzRead(): Not BZIP2 data");
    case BZ_MEM_ERROR:     	FAIL("BZ2_bzRead(): Insufficient memory");
    default:
                                FAIL("BZ2_bzRead(): Unknown failure");
    }
    /* NOT REACHED */
  }

  if (size - len < 16) {
    if (NULL == (newbuf = realloc(cpu68k_rom, size += 16)))
      FAIL("Out of memory!");

    cpu68k_rom = newbuf;
    memset(cpu68k_rom + len, 0, 16);
  }
  cpu68k_romlen = len;
  LOG_VERBOSE(("%d bytes loaded with BZIP2", cpu68k_romlen));

  return NULL;

failure:

  if (cpu68k_rom) {
    free(cpu68k_rom);
    cpu68k_rom = NULL;
  }
  cpu68k_romlen = 0;
  if (bzf) {
    BZ2_bzReadClose(&bzerror, bzf);
    bzf = NULL;
  }
  return error;
#undef FAIL
}
#endif /* USE_BZIP2 */

#ifdef USE_ZLIB
static char *loadimage_zlib(FILE *f)
{
  gzFile *gzf;
  void *newbuf = NULL;
  size_t len = 0, size = 0;
  int ret, err;
  int fd;
  char *error;

#define FAIL(x) do { error = (x); goto failure; } while (0)

  cpu68k_rom = NULL;
  cpu68k_romlen = 0;

  fd = dup(fileno(f));
  if ((gzf = gzdopen(fd, "rb")) == NULL) {
    close(fd);
    return ("gzopen() failed!");
  }

  for (;;) {
    if (size - len < REALLOC_THRESH) {
      if (size > MAX_ROM_SIZE)
        FAIL("ROM is too large");
        
      if (NULL == (newbuf = realloc(cpu68k_rom, size + REALLOC_INCR)))
        FAIL("Out of memory!");

      cpu68k_rom = newbuf;
      memset(cpu68k_rom + size, 0, REALLOC_INCR);
      size += REALLOC_INCR;
    }

    if ((ret = gzread(gzf, cpu68k_rom + len, REALLOC_THRESH)) <= 0)
      break;

    len += ret;
  }

  if (ret == -1) {
    gzerror(gzf, &err);
    switch (err) {
    case Z_ERRNO: 		FAIL(strerror(errno));
    case Z_STREAM_END:    	FAIL("gzread(): End of stream");
    case Z_NEED_DICT:   	FAIL("gzread(): Need dictionary");
    case Z_DATA_ERROR:  	FAIL("gzread(): Data error");
    case Z_MEM_ERROR:   	FAIL("gzread(): Memory error");
    case Z_BUF_ERROR:   	FAIL("gzread(): Buffer error");
    case Z_VERSION_ERROR:	FAIL("gzread(): Version error");
    default: 
			  	FAIL("gzread(): Unknown failure");
    }
    /* NOT REACHED */
  }
  gzclose(gzf);
  gzf = NULL;

  if (size - len < 16) {
    if (NULL == (newbuf = realloc(cpu68k_rom, size += 16)))
      FAIL("Out of memory!");

    cpu68k_rom = newbuf;
    memset(cpu68k_rom + len, 0, 16);
  }
  cpu68k_romlen = len;
  LOG_VERBOSE(("%d bytes loaded with ZLIB", cpu68k_romlen));

  return NULL;

failure:

  if (cpu68k_rom) {
    free(cpu68k_rom);
    cpu68k_rom = NULL;
  }
  cpu68k_romlen = 0;
  if (gzf) {
    gzclose(gzf);
    gzf = NULL;
  }
  return error;
#undef FAIL
}

static char *loadimage_deflate(FILE *f, size_t length, size_t full_length)
{
  z_stream zs;
  int ret, first = 1, zs_initialized = 0;
  char *buf;
  size_t len = 0;

  assert(f != NULL);
  assert(length != 0);

  if (NULL == (buf = calloc(1, length))) {
    LOG_NORMAL(("calloc(1, %lu) failed: %s", (unsigned long) length,
	strerror(errno)));
    goto failure;
  }
  while (len < length) {
    size_t bytes;

    bytes = fread(&buf[len], 1, length - len, f);
    if (0 == bytes)
      break;

    len += bytes;
  }
  len = 0;

  cpu68k_romlen = full_length + 16;
  if (NULL == (cpu68k_rom = malloc(cpu68k_romlen))) { 
    LOG_NORMAL(("malloc() failed: %s", strerror(errno)));
    goto failure;
  }

  zs.next_in = NULL;
  zs.avail_in = 0;
  zs.next_out = cpu68k_rom;
  zs.avail_out = full_length;
  zs.zalloc = NULL;
  zs.zfree = NULL;
  zs.opaque = NULL;
  ret = inflateInit2(&zs, -15);
  if (ret != Z_OK) {
    LOG_NORMAL(("inflateInit2() failed(): (ret=%d) %s",
	ret, zs.msg ? zs.msg : "Unknown error"));
    goto failure;
  }
  zs_initialized = 1;

  zs.next_in = cast_to_void_ptr(buf);
  zs.avail_in = length;

  for (;;) {
    ret = inflate(&zs, Z_SYNC_FLUSH);
    if (ret == Z_STREAM_END)
      break;

    if (ret == Z_DATA_ERROR) {
      ret = inflateSync(&zs);
      if (ret != Z_OK) {
        LOG_NORMAL(("inflateSync() failed: ret=%d", ret));
        goto failure;
      }
    } else if (ret == Z_BUF_ERROR) {

      if (zs.avail_out == 0) {
        if (first) {
          zs.next_in = cast_to_void_ptr("");
          zs.avail_in = 1;
        } 
      }

    } else if (ret != Z_OK) {
      LOG_NORMAL(("inflate failed(): (ret=%d) %s",
	ret,
	zs.msg ? zs.msg : "Unknown error"));
      goto failure;
    }
  }

  inflateEnd(&zs);
  free(buf);
  buf = NULL;
  LOG_VERBOSE(("%d bytes loaded with ZLIB", cpu68k_romlen));

  return NULL;

failure:

  cpu68k_romlen = 0;
  if (cpu68k_rom) {
    free(cpu68k_rom);
    cpu68k_rom = NULL;
  }
  if (buf) {
    free(buf);
    buf = NULL;
  }
  if (zs_initialized) {
    inflateEnd(&zs);
  }
  return "loadimage_deflate() failed";
}
#endif /* USE_ZLIB */


static char *loadimage_normal(FILE *f)
{
  struct stat sb;
  uint8 *buffer;
  size_t bytes, bytesleft;

  if (fstat(fileno(f), &sb) != 0)
    return ("Unable to stat file.");

  if (sb.st_size > MAX_ROM_SIZE)
    return ("File is too large");

  /* allocate enough memory plus 16 bytes for disassembler to cope
     with the last instruction */
  cpu68k_romlen = sb.st_size;
  if ((cpu68k_rom = calloc(1, cpu68k_romlen + 16)) == NULL) {
    cpu68k_romlen = 0;
    return ("Out of memory!");
  }
  
  buffer = cpu68k_rom;
  bytesleft = cpu68k_romlen;
  do {
    bytes = fread(buffer, 1, bytesleft, f);
    buffer += bytes;
    bytesleft -= bytes;
    if (ferror(f))
      return ("Error during fread()");
  } while (bytesleft > 0 && !feof(f));

  if (bytesleft) {
    LOG_CRITICAL(("%d bytes left to read?!", bytesleft));
    return ("Error whilst loading file");
  }

  LOG_VERBOSE(("%d bytes loaded", cpu68k_romlen));
  return NULL; 
}

/*** ZIP file support ***/

#define ZIP_FILE_MAGIC        0x04034b50UL
#define ZIP_DIR_MAGIC         0x02014b50UL
#define ZIP_DIR_END_MAGIC     0x06054b50UL

#define ZIP_FILE_HEADER_SIZE      26
#define ZIP_DIR_HEADER_SIZE       42
#define ZIP_DIR_END_HEADER_SIZE   18

#define ZIP_ENCODING_IDENTITY 0
#define ZIP_ENCODING_DEFLATE  8
#define ZIP_ENCODING_BZIP2    12

static const char *
zip_encoding_descr(unsigned int id)
{
  switch (id) {
  case ZIP_ENCODING_IDENTITY: return "Identity";
  case ZIP_ENCODING_DEFLATE:  return "Deflate";
  case ZIP_ENCODING_BZIP2:    return "BZIP2";
  }
  return "Unknown";
}

static int
zip_handle_dir_end_chunk(FILE *f)
{
  size_t ret, size;
  char header[ZIP_DIR_END_HEADER_SIZE];
  unsigned long data_size;
  unsigned int comment_len;
  char comment_buf[1024];
  off_t seek_pos;

  assert(f != NULL);

  size = sizeof header;
  assert(size <= ZIP_DIR_END_HEADER_SIZE);
  ret = fread(header, 1, size, f);
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP end-of-dir chunk: Too short\n"));
    return -1; 
  }

  data_size = peek_le32(&header[8]);
  comment_len = peek_le16(&header[16]);

  size = MIN(comment_len, sizeof comment_buf - 1);
  ret = fread(comment_buf, 1, size, f);
  comment_buf[size] = '\0';
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP end-of-dir chunk: File is truncated\n"));
    return -1; 
  }

  LOG_VERBOSE(("\n"
      "type:                  end of directory\n"
      "comment:               %s%s%s%s\n"
      "\n",
      comment_len ? "\"" : "<",
      comment_len ? comment_buf : "none",
      comment_len ? "\"" : ">",
      comment_len >= sizeof comment_buf ? " (truncated)" : ""));

  seek_pos = data_size;
  if (comment_len >= sizeof comment_buf)
    seek_pos += comment_len - sizeof comment_buf + 1;

  if (seek_pos >= LONG_MAX || seek_pos < 0) {
    LOG_NORMAL(("File is too large\n"));
    return -1;
  }

  if (seek_pos != 0 && fseek(f, seek_pos, SEEK_CUR)) {
    LOG_NORMAL(("fseek() failed: %s\n", strerror(errno)));
    return -1;
  }

  return 0;
}

static int
zip_handle_dir_chunk(FILE *f)
{
  size_t ret, size;
  char header[ZIP_DIR_HEADER_SIZE];
  unsigned long compressed_filesize, filesize, local_offset;
  unsigned int filename_len, extra_len, comment_len, encoding;
  char filename_buf[1024];
  char comment_buf[1024];
  off_t seek_pos;
  
  assert(f != NULL);

  size = sizeof header;
  assert(size <= ZIP_DIR_HEADER_SIZE);
  ret = fread(header, 1, size, f);
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP directory chunk: Too short\n"));
    return -1; 
  }

  encoding = peek_le16(&header[6]);
  compressed_filesize = peek_le32(&header[16]);
  filesize = peek_le32(&header[20]);
  filename_len = peek_le16(&header[24]);
  extra_len = peek_le16(&header[26]);
  comment_len = peek_le16(&header[28]);
  local_offset = peek_le32(&header[38]);

  size = MIN(filename_len, sizeof filename_buf - 1);
  ret = fread(filename_buf, 1, size, f);
  filename_buf[size] = '\0';
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP directory chunk: File is truncated\n"));
    return -1; 
  }
  
  size = MIN(comment_len, sizeof comment_buf - 1);
  ret = fread(comment_buf, 1, size, f);
  comment_buf[size] = '\0';
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP directory chunk: File is truncated\n"));
    return -1; 
  }
  
  LOG_VERBOSE(("\n"
      "type:                  directory\n"
      "filename:              %s%s%s%s\n"
      "filesize (real):       %lu\n"
      "filesize (compressed): %lu\n"
      "extra info length:     %u\n"
      "comment:               %s%s%s%s\n"
      "local offset:          %lu\n"
      "encoding:              %s\n"
      "\n",
      filename_len ? "\"" : "<",
      filename_len ? filename_buf : "none",
      filename_len ? "\"" : ">",
      filename_len >= sizeof filename_buf ? " (truncated)" : "",
      filesize,
      compressed_filesize,
      extra_len,
      comment_len ? "\"" : "<",
      comment_len ? comment_buf : "none",
      comment_len ? "\"" : ">",
      comment_len >= sizeof comment_buf ? " (truncated)" : "",
      local_offset,
      zip_encoding_descr(encoding)));

  seek_pos = extra_len;
  if (filename_len >= sizeof filename_buf)
    seek_pos += filename_len - sizeof filename_buf + 1;
  if (comment_len >= sizeof comment_buf)
    seek_pos += comment_len - sizeof comment_buf + 1;
  
  if (seek_pos >= LONG_MAX || seek_pos < 0) {
    LOG_NORMAL(("Chunk is too large\n"));
    return -1;
  }
  
  if (seek_pos != 0 && fseek(f, seek_pos, SEEK_CUR)) {
    LOG_NORMAL(("fseek() failed: %s\n", strerror(errno)));
    return -1;
  }

  return 0;
}

static int
zip_handle_file_chunk(FILE *f,
	int *data_encoding, size_t *data_length, size_t *full_length)
{
  size_t ret, size;
  char header[ZIP_FILE_HEADER_SIZE];
  unsigned long compressed_filesize, filesize;
  unsigned int filename_len, extra_len, encoding;
  char filename_buf[1024];
  off_t seek_pos;
  
  assert(f != NULL);

  if (data_length != NULL)
    *data_length = 0;
  if (full_length != NULL)
    *full_length = 0;
  if (data_encoding != NULL)
    *data_encoding = -1;

  size = sizeof header;
  assert(size <= ZIP_FILE_HEADER_SIZE);
  ret = fread(header, 1, size, f);
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP file chunk: Too short\n"));
    return -1; 
  }

  encoding = peek_le16(&header[4]);
  compressed_filesize = peek_le32(&header[14]);
  filesize = peek_le32(&header[18]);
  filename_len = peek_le16(&header[22]);
  extra_len = peek_le16(&header[24]);

  size = MIN(filename_len, sizeof filename_buf - 1);
  ret = fread(filename_buf, 1, size, f);
  filename_buf[size] = '\0';
  if (ret != size) {
    LOG_NORMAL(("Not a ZIP file: File is truncated\n"));
    return -1; 
  }

  LOG_VERBOSE(("\n"
      "type:                  file\n"
      "filename:              %s%s%s%s\n"
      "filesize (real):       %lu\n"
      "filesize (compressed): %lu\n"
      "extra info length:     %u\n"
      "encoding:              %s\n"
      "\n",
      filename_len ? "\"" : "<",
      filename_len ? filename_buf : "none",
      filename_len ? "\"" : ">",
      filename_len >= sizeof filename_buf ? "(truncated)" : "",
      filesize,
      compressed_filesize,
      extra_len,
      zip_encoding_descr(encoding)));

  seek_pos = extra_len;
  if (filename_len >= sizeof filename_buf)
    seek_pos += filename_len - sizeof filename_buf + 1;
  
  if (seek_pos >= LONG_MAX || seek_pos < 0) {
    LOG_NORMAL(("Chunk is too large\n"));
    return -1;
  }
  
  if (seek_pos != 0 && fseek(f, seek_pos, SEEK_CUR)) {
    LOG_NORMAL(("fseek() failed: %s\n", strerror(errno)));
    return -1;
  }

  if (data_length)
    *data_length = compressed_filesize;
  if (full_length)
    *full_length = filesize;
  if (data_encoding)
    *data_encoding = encoding;
  return 0;
}

static int
zip_handle_chunk(FILE *f, int *encoding,
	size_t *data_length, size_t *full_length)
{
  size_t ret, size;
  unsigned long magic;
  char magic_buf[4];
  
  assert(f != NULL);

  if (data_length != NULL)
    *data_length = 0;
  if (full_length != NULL)
    *full_length = 0;
  if (encoding != NULL)
    *encoding = -1;

  size = sizeof magic_buf;
  ret = fread(magic_buf, 1, size, f);
  if (0 == ret) {
    LOG_VERBOSE(("EOF"));
    return -1;
  }

  if (ret != size) {
    LOG_NORMAL(("Not a ZIP chunk: Too short\n"));
    return -1; 
  }
  
  magic = peek_le32(&magic_buf[0]);
  LOG_DEBUG1(("offset=0x%08lx\n", (unsigned long) ftell(f)));
  switch (magic) {
  case ZIP_FILE_MAGIC:
    return zip_handle_file_chunk(f, encoding, data_length, full_length);
  case ZIP_DIR_MAGIC:
    return zip_handle_dir_chunk(f);
  case ZIP_DIR_END_MAGIC:
    return zip_handle_dir_end_chunk(f);
  default:
    LOG_NORMAL(("Unknown ZIP chunk: magic=0x%08lx\n", magic));
  }
  return -1; 
}

static char * 
loadimage_zip(FILE *f)
{

  for (;;) {
    size_t data_length, full_length;
    int data_encoding;
    off_t offset, next;

    if (zip_handle_chunk(f, &data_encoding, &data_length, &full_length))
      return "Loading ZIP failed";

    offset = ftell(f); /* Store current position in file */

    if (full_length >= MIN_ROM_SIZE && full_length <= MAX_ROM_SIZE) {
      int success;
      const char *error;

      success = 0;
      switch (data_encoding) {
      case ZIP_ENCODING_IDENTITY:
        LOG_VERBOSE(("Found uncompressed ZIP chunk"));
        error = loadimage_normal(f);
        if (!error) {
          success = 1;
        } else {
          LOG_NORMAL(("%s", error));
        }
        break;

      case ZIP_ENCODING_DEFLATE:
#ifdef USE_ZLIB
        LOG_VERBOSE(("Found deflate encoded ZIP chunk"));
        error = loadimage_deflate(f, data_length, full_length);
        if (!error) {
          success = 1;
        } else {
          LOG_NORMAL(("%s", error));
        }
#else
        LOG_NORMAL(("ZIP contains a deflate compressed chunk but "
          "generator was compiled without ZLib support"));
#endif
        break;

      case ZIP_ENCODING_BZIP2:
        LOG_VERBOSE(("Found BZIP2 encoded ZIP chunk"));
#ifdef USE_BZIP2
        error = loadimage_bzip2(f);
        if (!error) {
          success = 1;
        } else {
          LOG_NORMAL(("%s", error));
        }
#else
        LOG_NORMAL(("ZIP contains a BZIP2 compressed chunk but "
          "generator was compiled without BZIP2 support"));
#endif
        break;

      default:
        LOG_NORMAL(("ZIP file contains chunk with unsupported encoding"));
      }

      if (success && cpu68k_romlen < 65536) {
        if (cpu68k_rom) {
          free(cpu68k_rom);
          cpu68k_rom = NULL;
        }
      }

      if (success) {
        LOG_VERBOSE(("Loaded data from ZIP"));
        return NULL;
      }

    }

    /* Move to next ZIP chunk */
    next = offset + data_length;
    if (next <= offset || fseek(f, offset, SEEK_SET)) {
      LOG_NORMAL(("fseek() failed: %s", strerror(errno)));
      return "Could not load data from ZIP file";
    }
  }
  
  return "Failed to load ROM from ZIP";
}

/*** gen_loadimage - load ROM image ***/

char *gen_loadimage(const char *filename)
{
  int imagetype;
  const char *extension;
  unsigned int blocks, x, i;
  uint8 *new;
  FILE *f;
  char *ret = NULL;
  int fd;

  /* Remove current file */
  if (cpu68k_rom) {
    if (gen_freerom)
      free(cpu68k_rom);
    cpu68k_rom = NULL;
  }
  cpu68k_romlen = 0;
  gen_freerom = 1;

#ifdef ALLEGRO
  if ((fd = open(filename, O_RDONLY | O_BINARY, 0)) == -1) {
#else
  if ((fd = open(filename, O_RDONLY, 0)) == -1) {
#endif
    perror("open");
    return ("Unable to open file.");
  }

  /* Next, try loading as ZIP archive */
  f = fdopen(dup(fd), "rb");
  if (!cpu68k_rom && (ret = loadimage_zip(f)) != NULL) {
    LOG_NORMAL(("%s", ret));
    fclose(f);
    if (lseek(fd, 0, SEEK_SET)) {
      perror("lseek");
      close(fd);
      return ("Could not seek to start of file");
    }
  }

#ifdef USE_BZIP2
  /* First, try loading as BZIP2 compressed file */
  f = fdopen(dup(fd), "rb");
  if (!cpu68k_rom && (ret = loadimage_bzip2(f)) != NULL) {
    LOG_NORMAL(("%s", ret));
    fclose(f);
    if (lseek(fd, 0, SEEK_SET)) {
      perror("lseek");
      close(fd);
      return ("Could not seek to start of file");
    }
  }
#endif /* USE_BZIP2 */

#ifdef USE_ZLIB
  /* Next, try loading as ZLIB compressed file */
  f = fdopen(dup(fd), "rb");
  if (!cpu68k_rom && (ret = loadimage_zlib(f)) != NULL) {
    LOG_NORMAL(("%s", ret));
    fclose(f);
    if (lseek(fd, 0, SEEK_SET)) {
      perror("lseek");
      close(fd);
      return ("Could not seek to start of file");
    }
  }
#endif /* USE_ZLIB */


  /* At last, try loading as uncompressed file */
  f = fdopen(dup(fd), "rb");
  if (!cpu68k_rom && (ret = loadimage_normal(f)) != NULL) {
    LOG_CRITICAL((ret));
    fclose(f);
    close(fd);
    return ret;
  }

  fclose(f);
  f = NULL;
  close(fd);

  if (cpu68k_romlen < 0x200) {
    if (cpu68k_rom) {
      cpu68k_romlen = 0;
      free(cpu68k_rom);
      cpu68k_rom = NULL;
    }
    return ("File is too small");
  }

  imagetype = 1;                /* BIN file by default */

  /* SMD file format check - Richard Bannister */
  if ((cpu68k_rom[8] == 0xAA) && (cpu68k_rom[9] == 0xBB) &&
      cpu68k_rom[10] == 0x06) {
    imagetype = 2;              /* SMD file */
  }
  /* check for interleaved 'SEGA' */
  if (cpu68k_rom[0x280] == 'E' && cpu68k_rom[0x281] == 'A' &&
      cpu68k_rom[0x2280] == 'S' && cpu68k_rom[0x2281] == 'G') {
    imagetype = 2;              /* SMD file */
  }
  /* Check extension is not wrong */
  extension = filename + strlen(filename) - 3;
  if (extension > filename) {
    if (!strcasecmp(extension, "smd") && (imagetype != 2))
      LOG_REQUEST(("File extension (smd) does not match detected "
                   "type (bin)"));
    if (!strcasecmp(extension, "bin") && (imagetype != 1))
      LOG_REQUEST(("File extension (bin) does not match detected "
                   "type (smd)"));
  }

  /* convert to standard BIN file format */

  switch (imagetype) {
  case 1:                      /* BIN */
    break;
  case 2:                      /* SMD */
    blocks = (cpu68k_romlen - 512) / 16384;
    if (blocks * 16384 + 512 != cpu68k_romlen)
      return ("Image is corrupt.");

    if ((new = malloc(cpu68k_romlen - 512)) == NULL) {
      cpu68k_rom = NULL;
      cpu68k_romlen = 0;
      return ("Out of memory!");
    }

    for (i = 0; i < blocks; i++) {
      for (x = 0; x < 8192; x++) {
        new[i * 16384 + x * 2 + 0] = cpu68k_rom[512 + i * 16384 + 8192 + x];
        new[i * 16384 + x * 2 + 1] = cpu68k_rom[512 + i * 16384 + x];
      }
    }
    free(cpu68k_rom);
    cpu68k_rom = new;
    cpu68k_romlen -= 512;
    break;
  default:
    return ("Unknown image type");
    break;
  }

  /* is this icky? */
  {
    const char *path;
    char *p = gen_leafname;
    size_t size = sizeof gen_leafname;

    path = strrchr(filename, '/');
    if (!path)
      path = strrchr(filename, '\\');
    if (!path)
      path = filename;
    else
      path++; /* skip slash or backslash */
    
    p = append_string(p, &size, path);
    
    p = strrchr(gen_leafname, '.');
    if (p && (0 == strcasecmp(p, ".smd") || 0 == strcasecmp(p, ".bin")))
      *p = '\0';
    
    if ('\0' == gen_leafname[0]) {
      p = gen_leafname;
      size = sizeof gen_leafname;
      append_string(p, &size, "rom");
    }
  }


  gen_setupcartinfo();

  /*
   * Okay, if we have autodetect, let's still take into account the user
   * preferences. If there are more than one possible regions, selection
   * will be based on the config file's settings. -Trilkk
   */
  if (gen_autodetect) {
    /* If our flags allow our preferences, no need for guessing. */
    if (
	!((vdp_overseas && vdp_pal && gen_cartinfo.flag_europe) ||
	  (vdp_overseas && !vdp_pal && gen_cartinfo.flag_usa) ||
	  (!vdp_overseas && !vdp_pal && gen_cartinfo.flag_japan))
    ) {
      /* Otherwise resort to the old system. */
      vdp_pal = gen_cartinfo.flag_europe &&
	!(gen_cartinfo.flag_usa || gen_cartinfo.flag_japan);
      vdp_overseas = !gen_cartinfo.flag_japan &&
	(gen_cartinfo.flag_usa || gen_cartinfo.flag_europe);
    }
    LOG_REQUEST(("USA: %d; Japan: %d; Europe: %d; TV Mode: %s",
      gen_cartinfo.flag_usa, gen_cartinfo.flag_japan, gen_cartinfo.flag_europe,
      vdp_pal ? "PAL" : "NTSC"));
  }

  /* reset system */
  gen_reset();

  if (gen_cartinfo.checksum != (cpu68k_rom[0x18e] << 8 | cpu68k_rom[0x18f]))
    LOG_REQUEST(("Warning: Checksum does not match in ROM (%04X)",
                 (cpu68k_rom[0x18e] << 8 | cpu68k_rom[0x18f])));

  LOG_REQUEST(("Loaded '%s'/'%s' (%s %04X %s)", gen_cartinfo.name_domestic,
               gen_cartinfo.name_overseas, gen_cartinfo.version,
               gen_cartinfo.checksum, gen_cartinfo.country));

  gen_modifiedrom = 0;
  return NULL;
}

/* setup to run from ROM in memory */

void gen_loadmemrom(const void *rom, int romlen)
{
  cpu68k_rom = deconstify_void_ptr(rom); /* I won't alter it, promise */
  cpu68k_romlen = romlen;
  gen_freerom = 0;
  gen_setupcartinfo();
  gen_reset();
}

/* setup gen_cartinfo from current loaded rom */

void gen_setupcartinfo(void)
{
  unsigned int i;
  char *p;

  memset(&gen_cartinfo, 0, sizeof(gen_cartinfo));
  gen_nicetext(gen_cartinfo.console, (char *)(cpu68k_rom + 0x100), 16);
  gen_nicetext(gen_cartinfo.copyright, (char *)(cpu68k_rom + 0x110), 16);
  gen_nicetext(gen_cartinfo.name_domestic, (char *)(cpu68k_rom + 0x120), 48);
  gen_nicetext(gen_cartinfo.name_overseas, (char *)(cpu68k_rom + 0x150), 48);
  if (cpu68k_rom[0x180] == 'G' && cpu68k_rom[0x181] == 'M') {
    gen_cartinfo.prodtype = pt_game;
  } else if (cpu68k_rom[0x180] == 'A' && cpu68k_rom[0x181] == 'I') {
    gen_cartinfo.prodtype = pt_education;
  } else {
    gen_cartinfo.prodtype = pt_unknown;
  }
  gen_nicetext(gen_cartinfo.version, (char *)(cpu68k_rom + 0x182), 12);
  gen_cartinfo.checksum = gen_checksum(((uint8 *)cpu68k_rom) + 0x200,
                                       cpu68k_romlen - 0x200);
  gen_nicetext(gen_cartinfo.memo, (char *)(cpu68k_rom + 0x1C8), 28);

  /*
   * For more info, go here:
   * http://www.genesiscollective.com/faq.php?myfaq=yes&id_cat=2&categories=Game+Cartridge+And+ROM+Questions
   *  -Trilkk
   */
   for (i = 0x1f0; i < 0x1ff; i++)
     switch (cpu68k_rom[i]) {
     case 'J':
       gen_cartinfo.flag_japan = 1;
       break;
     case 'U':
       gen_cartinfo.flag_usa = 1;
       break;
     case 'E':
       gen_cartinfo.flag_europe = 1;
       break;
     }

  /*
   * FIXME: Please note that 'E' is used on bot old and new region standard.
   * This might cause some serious trouble for old games that the emulator
   * would think to be usable in other configurations than pal-e. Thus, we
   * should somehow check for the year of the game, and do the following test
   * only if it is post-1994. Other way around is not dangerous, since 'E'
   * contains europe anyway.
   */
  if (cpu68k_rom[0x1f0] >= '1' && cpu68k_rom[0x1f0] <= '9') {
    gen_cartinfo.hardware = cpu68k_rom[0x1f0] - '0';
  } else if (cpu68k_rom[0x1f0] >= 'A' && cpu68k_rom[0x1f0] <= 'F') {
    gen_cartinfo.hardware = cpu68k_rom[0x1f0] - 'A' + 10;
  }
  /* New machine code represents a bitmask, we test it here: */
  if (gen_cartinfo.hardware & 0x01) /* ntsc-j */
    gen_cartinfo.flag_japan = 1;
  /* if (gen_cartingo.hardware & 0x02)*/ /* pal-j, disregard this one */
  if (gen_cartinfo.hardware & 0x04) /* ntsc-u */
    gen_cartinfo.flag_usa = 1;
  if (gen_cartinfo.hardware & 0x08) /* pal-e */
    gen_cartinfo.flag_europe = 1;
  /* FIXME: End of post-1994 test. */

  /* Get the country string. */
  p = gen_cartinfo.country;
  for (i = 0x1f0; i < 0x200; i++) {
    if (cpu68k_rom[i] != 0 && cpu68k_rom[i] != 32)
      *p++ = cpu68k_rom[i];
  }
  *p = '\0';
  LOG_REQUEST(("country=%s\n", gen_cartinfo.country));
}

/*** get_nicetext - take a string, remove spaces and capitalise ***/

void gen_nicetext(char *out, char *in, unsigned int size)
{
  int flag, i;
  int c;
  char *start = out;

  flag = 0;                     /* set if within word, e.g. make lowercase */
  i = size;                     /* maximum number of chars in input */
  while ((c = *(unsigned char *)in++) && --i > 0) {
    if (isalpha(c)) {
      if (!flag) {
        /* make uppercase */
        flag = 1;
        if (islower(c))
          *out++ = c - 'z' + 'Z';
        else
          *out++ = c;
      } else {
        /* make lowercase */
        if (isupper(c))
          *out++ = (c) - 'Z' + 'z';
        else
          *out++ = c;
      }
    } else if (c == ' ') {
      if (flag)
        *out++ = c;
      flag = 0;
    } else if (isprint(c) && c != '\t') {
      flag = 0;
      *out++ = c;
    }
  }
  while (out > start && ' ' == *(out - 1))
    out--;
  *out++ = '\0';
}

/*** gen_checksum - get Genesis-style checksum of memory block ***/

uint16 gen_checksum(uint8 *start, unsigned int length)
{
  uint16 checksum;

  if (length & 1) {
    length &= ~1;
    LOG_VERBOSE(("checksum routines given odd length (%d)", length));
  }

  for (checksum = 0; length; length -= 2, start += 2) {
    checksum += peek_be16(start);
  }
  return checksum;
}

/* vi: set ts=2 sw=2 et cindent: */
