#include "GrMonitor.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "ShrikeCanvas.hpp"

using namespace SH;

GrMonitor::GrMonitor(double x, double y)
  : m_x(x), m_y(y),
    m_width(80), m_height(80),
    m_fp(0), m_vp(0)
{
}

void GrMonitor::draw()
{
  
  GLdouble mv[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mv);
  GLdouble proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  int oldvp[4];
  glGetIntegerv(GL_VIEWPORT, oldvp);

  glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_SCISSOR_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double x1, y1, z1;
  double x2, y2, z2;
  
  gluProject(m_x, m_y, 0.0, mv, proj, oldvp, &x1, &y1, &z1);
  gluProject(m_x + m_width, m_y + m_height, 0.0, mv, proj, oldvp, &x2, &y2, &z2);

  if (x1 < 0.0) x1 = 0.0;
  if (x2 < 0.0) x2 = 0.0;
  if (y1 < 0.0) y1 = 0.0;
  if (y2 < 0.0) y2 = 0.0;
  if (x1 > oldvp[2]) x1 = oldvp[2];
  if (x2 > oldvp[2]) x2 = oldvp[2];
  if (y1 > oldvp[3]) y1 = oldvp[3];
  if (y2 > oldvp[3]) y2 = oldvp[3];
  
  glScissor((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1));

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_DEPTH_TEST);
  
  if (m_vp) {
    std::cerr << "Binding monitor vertex program:" << std::endl;
    m_vp->code()->print(std::cerr);
    shBindShader(m_vp);
  }
  if (m_fp) {
    std::cerr << "Binding monitor fragment program:" << std::endl;
    m_fp->code()->print(std::cerr);
    shBindShader(m_fp);
  }
  
  glEnable(GL_VERTEX_PROGRAM_ARB);
  glEnable(GL_FRAGMENT_PROGRAM_ARB);
  
  ShrikeCanvas::instance()->setupViewCur();
  glViewport((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1));
  ShrikeCanvas::instance()->renderCur();

  glDisable(GL_VERTEX_PROGRAM_ARB);
  glDisable(GL_FRAGMENT_PROGRAM_ARB);
  
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();

  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_LOOP); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y + m_height, 0.0);
    glVertex3f(m_x, m_y + m_height, 0.0);
  } glEnd();
}
