#include "GrNode.hpp"
#define OGLFT_NO_SOLID
#define OGLFT_NO_QT
#include <oglft/OGLFT.h>
#include <GL/gl.h>
#include <sh/sh.hpp>
#include "GrPort.hpp"
#include "GrView.hpp"

using namespace SH;

namespace {
const float border = 2.0;
const float port_width = 8.0;
const float shadow = 2.0;
const float conn_radius = 3.0;
const float conn_sep = conn_radius/2.0;
const float io_sep = 10.0;

std::string label(const SH::ShVariableNodePtr& node) {
  return node->hasName() ? node->name() : "noname";
}

}


GrNode::GrNode(const SH::ShProgram& program,
               double x, double y,
               OGLFT::Face* face,
               GrView* view)
  : m_program(program),
    m_x(x), m_y(y),
    m_face(face),
    m_view(view),
    m_gl_name(view->addPicker(PICK_NODE, this)),
    m_marked(false)
{

  calcSizes();

  ShProgramNode::VarList::const_iterator I;
  { // Set up ports
    double y;
    y = m_height - m_font_height - (m_height - m_font_height * m_program->inputs.size())/2.0;
    for (I = m_program->inputs.begin(); I != m_program->inputs.end(); ++I) {
      m_input_ports.push_back(new GrPort(this, *I, border, y, true));
      y -= m_font_height;
    }
    
    y = m_height - m_font_height - (m_height - m_font_height * m_program->outputs.size())/2.0;
    for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I) {
      m_output_ports.push_back(new GrPort(this, *I, - border + m_width - port_width, y, false));
      y -= m_font_height;
    }
  }
}

GrNode::~GrNode()
{
  for (PortList::iterator I = m_input_ports.begin(); I != m_input_ports.end(); ++I) {
    delete *I;
  }
  for (PortList::iterator I = m_output_ports.begin(); I != m_output_ports.end(); ++I) {
    delete *I;
  }
}

void GrNode::clearMarked()
{
  if (!m_marked) return;

  m_marked = false;

  std::cerr << "Marking something as clear" << std::endl;
  
  // TODO: This is hideous. Maybe I should rethink the graph
  // representation.
  for (PortList::iterator I = m_input_ports.begin(); I != m_input_ports.end(); ++I) {
    GrPort* port = *I;
    for (GrPort::EdgeList::iterator J = port->begin_edges(); J != port->end_edges(); ++J) {
      GrEdge* edge = *J;
      if (edge->to != port) {
        std::cerr << "Skipping something" << std::endl;
        continue;
      }
      std::cerr << "Marking something else as clear" << std::endl;
      edge->from->parent()->clearMarked();
    }
  }
}

void GrNode::calcSizes()
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
    OGLFT::BBox bbox = m_face->measure(label(*I).c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_in_width) m_in_width = width;
  }
  
  m_out_width = 0;
  for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I) {
    OGLFT::BBox bbox = m_face->measure(label(*I).c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_out_width) m_out_width = width;
  }

  m_width = m_in_width + m_out_width;

  if (!m_program->name().empty()) {
    OGLFT::BBox bbox = m_face->measure(m_program->name().c_str());
    double width = bbox.x_max_ - bbox.x_min_;
    if (width > m_width) m_width = width;
  }
  
  m_width += border * 2.0;
  m_width += port_width * 2.0;
  m_width += border * 2.0;
  m_height += border * 2.0;
  m_width += io_sep;

  { // Set up ports
    double y;
    y = m_height - m_font_height - (m_height - m_font_height * m_program->inputs.size())/2.0;
    for (PortList::iterator I = m_input_ports.begin(); I != m_input_ports.end(); ++I) {
      (*I)->move(border, y);
      y -= m_font_height;
    }
    
    y = m_height - m_font_height - (m_height - m_font_height * m_program->outputs.size())/2.0;
    for (PortList::iterator I = m_output_ports.begin(); I != m_output_ports.end(); ++I) {
      (*I)->move(- border + m_width - port_width, y);
      y -= m_font_height;
    }
  }
}

void GrNode::combine(const ShProgram& program)
{
 
  int prev_inputs = m_program->inputs.size();
  int prev_outputs = m_program->outputs.size();
 
  std::string name = m_program->name();
  m_program = m_program & program;
  m_program->name(name);

  // TODO: operator& should probably keep the name.
  
  calcSizes();

  ShProgramNode::VarList::const_iterator I;
  { // Set up ports
    double y;
    y = m_height - m_font_height - (m_height - m_font_height * m_program->inputs.size())/2.0;
    int i = 0;
    for (I = m_program->inputs.begin(); I != m_program->inputs.end(); ++I, ++i) {
      if (i < prev_inputs) {
        m_input_ports[i]->move(border, y);
      } else {
        m_input_ports.push_back(new GrPort(this, *I, border, y, true));
      }
      y -= m_font_height;
    }

    i = 0;
    y = m_height - m_font_height - (m_height - m_font_height * m_program->outputs.size())/2.0;
    for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I, ++i) {
      if (i < prev_outputs) {
        m_output_ports[i]->move(m_width - border - port_width, y);
      } else {
        m_output_ports.push_back(new GrPort(this, *I, m_width - border - port_width, y, false));
      }
      y -= m_font_height;
    }
  }
  
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
    glVertex3f(m_x + border + m_in_width + port_width + io_sep/2.0, m_y, 0.0);
    glVertex3f(m_x + border + m_in_width + port_width + io_sep/2.0, m_y + m_height, 0.0);
  } glEnd();

  // Outline
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINE_LOOP); {
    glVertex3f(m_x, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y, 0.0);
    glVertex3f(m_x + m_width, m_y + m_height, 0.0);
    glVertex3f(m_x, m_y + m_height, 0.0);
  } glEnd();

  for (PortList::iterator I = m_input_ports.begin(); I != m_input_ports.end(); ++I) {
    (*I)->draw();
  }
  for (PortList::iterator I = m_output_ports.begin(); I != m_output_ports.end(); ++I) {
    (*I)->draw();
  }
  
  glPopAttrib();
  glPopName();
}

void GrNode::draw_edges()
{
  for (PortList::iterator I = m_output_ports.begin(); I != m_output_ports.end(); ++I) {
    (*I)->draw_edges();
  }
}

void GrNode::draw_labels()
{
  ShProgramNode::VarList::const_iterator I;
  
  double y;
  y = m_y + m_height - m_font_height - (m_height - m_font_height * m_program->inputs.size())/2.0;
  
  m_face->setHorizontalJustification( OGLFT::Face::LEFT );
  for (I = m_program->inputs.begin(); I != m_program->inputs.end(); ++I) {
    m_face->draw(m_x + border * 2.0 + port_width, y, label(*I).c_str());
    y -= m_font_height;
  }

  m_face->setHorizontalJustification( OGLFT::Face::RIGHT );
  
  y = m_y + m_height - m_font_height - (m_height - m_font_height * m_program->outputs.size())/2.0;
  for (I = m_program->outputs.begin(); I != m_program->outputs.end(); ++I) {
    m_face->draw(m_x - border * 2.0 + m_width - port_width, y, label(*I).c_str());
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

