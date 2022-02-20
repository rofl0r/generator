#ifndef GEN_OPENGL_H
#define GEN_OPENGL_H

void opengl_setup(unsigned hsize, unsigned vsize, float scale);
void opengl_render32(void *buffer, unsigned hsize, unsigned vsize, unsigned pitch, float scale);

#endif

