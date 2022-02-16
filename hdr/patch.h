typedef struct _t_patchlist {
  struct _t_patchlist *next;
  char code[32];
  char *action;
} t_patchlist;

extern t_patchlist *patch_patchlist;

int patch_loadfile(const char *filename);
int patch_savefile(const char *filename);
void patch_clearlist(void);
void patch_addcode(const char *code, const char *action);
int patch_apply(const char *code, const char *action);
