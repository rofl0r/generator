#ifndef STATE_HEADER_FILE
#define STATE_HEADER_FILE

time_t state_date(const int slot);
int state_load(const int slot);
int state_save(const int slot);
int state_loadfile(const char *filename);
int state_savefile(const char *filename);

void state_write8(const char *mod, const char *name, uint8 instance,
                                    uint8 *data, uint32 size);
void state_write16(const char *mod, const char *name, uint8 instance,
                                     uint16 *data, uint32 size);
void state_write32(const char *mod, const char *name, uint8 instance,
                                     uint32 *data, uint32 size);

#endif /* STATE_HEADER_FILE */
