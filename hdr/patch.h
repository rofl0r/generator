#ifndef PATCH_HEADER_FILE
#define PATCH_HEADER_FILE

typedef struct _t_patchlist {
  struct _t_patchlist *next;
  char code[32]; /* Game Genie code */
  char *comment;
} t_patchlist;

extern t_patchlist *patch_patchlist;

int patch_loadfile(const char *filename);
int patch_savefile(const char *filename);
void patch_clearlist(void);
void patch_addcode(const char *code, const char *comment);
int patch_apply(const char *code);
int patch_genietoraw(const char *code, uint32 *addr, uint16 *data);

#endif /* PATCH_HEADER_FILE */
