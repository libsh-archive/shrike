#include "GrNode.hpp"
#define OGLFT_NO_SOLID
#define OGLFT_NO_QT
#include <oglft/OGLFT.h>
#include <GL/gl.h>
#include <sh/sh.hpp>

using namespace SH;

namespace {
const float border = 2.0;
const float shadow = 2.0;
const float conn_radius = 3.0;
const float conn_sep = conn_radius/2.0;
const float io_sep = 10.0;
}

unsigned int GrNode::m_max_gl_name = 0;

GrNode::GrNode(const SH::ShProgram& program,
               double x, double y,
               OGLFT::Face* face)
  : m_program(program),
    m_x(x), m_y(y),
    m_face(face),
    m_gl_name(m_max_gl_name++)
{
  // Compute width, height

  m_width = 0;
  m_height = 0;

  {
    OGLFT::BBox bbox = m_face->measure("Q");
    m_font_height = bbox.y_max_ - bbox.y_min_;
  }

  m_in_height = m_program->inputs.size() * m_font_height;
  if (m_in_height > m_height) m_height = m_in_height;
  m_out_height = m_program->outputs.size() * m_font_height;
  if (m_out_height > m_height) m_height = m_out_height;

  m_in_width = 0;
  ShProgramNode::VarList::const_iterator I;
  for (I = m_program->inputs.begin(); I != m_program->inputs.end(); ++I) {
    OGLFT::BBox bbox = m_face->measure((*I)->name().c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_in_width) m_in_width = width;
  }
  
  m_out_width = 0;
  for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I) {
    OGLFT::BBox bbox = m_face->measure((*I)->name().c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_out_width) m_out_width = width;
  }

  m_width = m_in_width + m_out_width;

  if (!program->name().empty()) {
    OGLFT::BBox bbox = m_face->measure(program->name().c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_width) m_width = width;
  }
  
  m_width += border * 2.0;
  m_height += border * 2.0;
  m_width += io_sep;
}

void GrNode::draw_box()
{
  glPushName(m_gl_name);
  glPushAttrib(GL_CURRENT_BIT);
  glColor3f(0.0, 0.0, 0.0);
  // Shadow
  double s_height = m_height + (m_program->name().empty() ? 0 : m_font_height + border + border);
  glBegin(GL_QUADS); {
    glVertex3f(m_x + shadow, m_y - shadow, 0.0);
    glVertex3f(m_x + m_width + shadow, m_y - shadow, 0.0);
    glVertex3f(m_x + m_width + shadow, m_y + s_height - shadow, 0.0);
    glVertex3f(m_x + shadow, m_y + s_height - shadow, 0.0);
  } glEnd();

  // Main region
  glColor3f(0.8, 0.8, 0.8);
  glBegin(GL_QUADS); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y + m_height, 0.0);
    glVertex3f(m_x, m_y + m_height, 0.0);
  } glEnd();

  // Title
  if (!m_program->name().empty()) {
    glColor3f(0.4, 0.6, 0.8);
    glBegin(GL_QUADS); {
      glVertex3f(m_x, m_y + m_height, 0.0);
      glVertex3f(m_x + m_width, m_y + m_height, 0.0);
      glVertex3f(m_x + m_width, m_y + m_height + m_font_height + border * 2.0, 0.0);
      glVertex3f(m_x, m_y + m_height + m_font_height + border * 2.0, 0.0);
    } glEnd();
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP); {
      glVertex3f(m_x, m_y + m_height, 0.0);
      glVertex3f(m_x + m_width, m_y + m_height, 0.0);
      glVertex3f(m_x + m_width, m_y + m_height + m_font_height + border * 2.0, 0.0);
      glVertex3f(m_x, m_y + m_height + m_font_height + border * 2.0, 0.0);
    } glEnd();
  }

  // Centre line
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_LINES); {
    glVertex3f(m_x + border + m_in_width + io_sep/2.0, m_y, 0.0);
    glVertex3f(m_x + border + m_in_width + io_sep/2.0, m_y + m_height, 0.0);
  } glEnd();

  // Outline
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_LOOP); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y + m_height, 0.0);
    glVertex3f(m_x, m_y + m_height, 0.0);
  } glEnd();
  
  glPopAttrib();
  glPopName();
}

void GrNode::draw_edges()
{
}

void GrNode::draw_labels()
{
  ShProgramNode::VarList::const_iterator I;
  
  double y;
  y = m_y + m_height - m_font_height - (m_height - m_font_height * m_program->inputs.size())/2.0;
  
  m_face->setHorizontalJustification( OGLFT::Face::LEFT );
  for (I = m_program->inputs.begin(); I != m_program->inputs.end(); ++I) {
    m_face->draw(m_x + border, y, (*I)->name().c_str());
    y -= m_font_height;
  }

  m_face->setHorizontalJustification( OGLFT::Face::RIGHT );
  
  y = m_y + m_height - m_font_height - (m_height - m_font_height * m_program->outputs.size())/2.0;
  for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I) {
    m_face->draw(m_x - border + m_width, y, (*I)->name().c_str());
    y -= m_font_height;
  }

  if (!m_program->name().empty()) {
    m_face->setHorizontalJustification( OGLFT::Face::CENTER );
    m_face->draw(m_x + m_width/2.0, m_y + m_height + border, m_program->name().c_str());
  }
  
}

void GrNode::moveTo(double x, double y)
{
  m_x = x; m_y = y;
}
