#ifndef GENSOUNDP_HEADER_FILE
#define GENSOUNDP_HEADER_FILE

int soundp_start(void);
void soundp_stop(void);
int soundp_samplesbuffered(void);
int soundp_output(uint16 *left, uint16 *right, unsigned int samples);

#endif /* GENSOUNDP_HEADER_FILE */
