#include "GrMonitor.hpp"
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "ShrikeCanvas.hpp"
#include "GrPort.hpp"
#include "GrNode.hpp"
#include "GrView.hpp"

using namespace SH;

namespace {

void check_gl_error(char* desc)
{
  GLenum error;
  if ((error = glGetError()) != GL_NO_ERROR) {
    std::cerr << gluErrorString(error) << " at " << desc << std::endl;
  }
}

}

#define CHECK_GL_ERROR(func) \
  do { \
    func; \
    check_gl_error( # func ); \
  } while (0)

GrMonitor::GrMonitor(GrPort* port)
  : m_port(port),
    m_x(port->global_x() + 10.0), m_y(port->global_y() + 10.0),
    m_width(80), m_height(80),
    m_gl_name(port->parent()->view()->addPicker(PICK_MONITOR, this)),
    m_vp(0), m_fp(0)
{
}

void GrMonitor::draw()
{
  int cwi, chi;
  ShrikeCanvas::instance()->GetClientSize(&cwi, &chi);
  double cw = (double)cwi;
  double ch = (double)chi;

  if (cw < ch) {
    m_height = 80.0 * ch/cw;
    m_width = 80.0;
  } else {
    m_height = 80.0;
    m_width = 80.0 * cw/ch;
  }
  
  glPushAttrib(GL_LINE_BIT); {
    glLineWidth(2.0);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0f0f);
    glColor3f(0.0, 0.0, 0.8);
    glBegin(GL_LINES); {
      glVertex3f(m_x + m_width/2.0, m_y + m_height/2.0, 0.0);
      glVertex3f(m_port->global_x(), m_port->global_y(), 0.0);
    } glEnd();
    glDisable(GL_LINE_STIPPLE);
  } glPopAttrib();
  
  if (m_vp && m_fp) {
  
    GLdouble mv[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    GLdouble proj[16];
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    int oldvp[4];
    glGetIntegerv(GL_VIEWPORT, oldvp);

    CHECK_GL_ERROR(glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_SCISSOR_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

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
    CHECK_GL_ERROR(glPushMatrix());
    glLoadIdentity();
  
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR(glPushMatrix());
    glLoadIdentity();

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
  
    //  m_vp->code()->print(std::cerr);
    shBindShader(m_vp);
  
    //  m_fp->code()->print(std::cerr);
    shBindShader(m_fp);
  
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
  
    ShrikeCanvas::instance()->setupViewCur();
    glViewport((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1));
    ShrikeCanvas::instance()->renderCur();

    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
  
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR(glPopMatrix());
  
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR(glPopMatrix());

    CHECK_GL_ERROR(glPopAttrib());

  } else {
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_QUADS); {
      glVertex3f(m_x, m_y, 0.0);
      glVertex3f(m_x + m_width - 1, m_y, 0.0);
      glVertex3f(m_x + m_width - 1, m_y + m_height - 1, 0.0);
      glVertex3f(m_x, m_y + m_height - 1, 0.0);
    } glEnd();

    glPushAttrib(GL_POLYGON_STIPPLE_BIT);

    GLubyte mask[32*32/sizeof(GLubyte)];
    for (int i = 0; i < 32*32/sizeof(GLubyte); i++) {
      mask[i] = (((i / 4) % 2) == 0 ? 0x55 : 0xaa);
    }
    
    glPolygonStipple(mask);
    glEnable(GL_POLYGON_STIPPLE);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_QUADS); {
      glVertex3f(m_x, m_y, 0.0);
      glVertex3f(m_x + m_width - 1, m_y, 0.0);
      glVertex3f(m_x + m_width - 1, m_y + m_height - 1, 0.0);
      glVertex3f(m_x, m_y + m_height - 1, 0.0);
    } glEnd();
    glDisable(GL_POLYGON_STIPPLE);

    glPopAttrib();
  }
  
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_LOOP); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width - 1, m_y, 0.0);
    glVertex3f(m_x + m_width - 1, m_y + m_height - 1, 0.0);
    glVertex3f(m_x, m_y + m_height - 1, 0.0);
  } glEnd();
}

void GrMonitor::pick()
{
  glPushName(m_gl_name);
  glBegin(GL_QUADS); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width - 1, m_y, 0.0);
    glVertex3f(m_x + m_width - 1, m_y + m_height - 1, 0.0);
    glVertex3f(m_x, m_y + m_height - 1, 0.0);
  } glEnd();
  glPopName();
}

void destroy(GrMonitor* monitor)
{
  GrPort* port = monitor->port();
  GrView* view = port->parent()->view();
  port->monitor(0);
  view->removeMonitor(monitor);
  delete monitor;
}
