#include "opengl.h"
#include <GL/glu.h>

static GLuint texture_id = 0;

void opengl_setup(unsigned hsize, unsigned vsize, float scale) {
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glViewport(0, 0, hsize*scale, hsize*scale);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_3D_EXT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, hsize, vsize, 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void opengl_render32(void *buffer, unsigned hsize, unsigned vsize, unsigned pitch, float scale)
{
  glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hsize, vsize, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
  glViewport(0, 0, hsize*scale, vsize*scale);

  glBegin(GL_QUADS);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(hsize, vsize);

  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(hsize, 0.0);

  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(0.0, 0.0);

  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(0, vsize);
  glEnd();
}
