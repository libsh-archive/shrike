#include "GrPort.hpp"
#include <GL/gl.h>
#include "GrView.hpp"

using namespace SH;

GrPort::GrPort(GrNode* parent, const SH::ShVariableNodePtr& var,
               double x, double y,
               bool in) // bottom-left coordinates
  : m_parent(parent), m_var(var),
    m_x(x), m_y(y),
    m_gl_name(parent->view()->addPicker(PICK_PORT, this)),
    m_in(in)
{
}

GrPort::~GrPort()
{
  for (EdgeList::iterator I = m_edges.begin(); I != m_edges.end(); ++I) {
    for (EdgeList::iterator J = I->to->m_edges.begin(); J != I->to->m_edges.end(); ++J) {
      if (I->from == J->from && I->to == J->to) {
        I->to->m_edges.erase(J);
        break;
      }
    }
  }
}

void GrPort::draw()
{
  double port_width = 8.0;

  double x = m_x + m_parent->x();
  double y = m_y + m_parent->y();

  glPushName(m_gl_name);
  
  glColor3f(1.0, 0.0, 1.0);
  glBegin(GL_QUADS); {
    glVertex3f(x, y, 0.1);
    glVertex3f(x, y + port_width, 0.1);
    glVertex3f(x + port_width, y + port_width, 0.1);
    glVertex3f(x + port_width, y, 0.1);
  } glEnd();

  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_LOOP); {
    glVertex3f(x, y, 0.1);
    glVertex3f(x, y + port_width, 0.1);
    glVertex3f(x + port_width, y + port_width, 0.1);
    glVertex3f(x + port_width, y, 0.1);
  } glEnd();
  
  glPopName();
}

double GrPort::global_x() const
{
  double port_width = 8.0;
  return m_x + m_parent->x() + port_width/2.0;
}
double GrPort::global_y() const
{
  double port_width = 8.0;
  return m_y + m_parent->y() + port_width/2.0;
}

void GrPort::draw_edges()
{
  for (EdgeList::const_iterator I = m_edges.begin(); I != m_edges.end(); ++I) {
    if (this != I->from) continue;
    double x_from = global_x();
    double y_from = global_y();
    double x_to = I->to->global_x();
    double y_to = I->to->global_y();

    draw_edge(x_from, y_from, x_to, y_to);
  }
}


void GrPort::draw_edge(double x_from, double y_from, double x_to, double y_to)
{
  double border = 2.0;
  double port_width = 8.0;
  
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_STRIP); {
    glVertex3f(x_from, y_from, 0.1);
    glVertex3f(x_from + port_width/2.0 + border * 2.0, y_from, 0.1);
    glVertex3f(x_to - port_width/2.0 - border * 2.0, y_to, 0.1);
    glVertex3f(x_to, y_to, 0.1);
  } glEnd();
}


void join(GrPort* a, GrPort* b)
{
  a->m_edges.push_back(GrEdge(a, b));
  b->m_edges.push_back(GrEdge(a, b));
}

void unjoin(GrPort* a, GrPort* b)
{
  for (GrPort::EdgeList::iterator I = a->m_edges.begin(); I != a->m_edges.end(); ++I) {
    if (I->from == a && I->to == b) {
      a->m_edges.erase(I);
      break;
    }
  }
  for (GrPort::EdgeList::iterator I = b->m_edges.begin(); I != b->m_edges.end(); ++I) {
    if (I->from == a && I->to == b) {
      b->m_edges.erase(I);
      break;
    }
  }
}
