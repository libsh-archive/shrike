#ifndef GRNODE_HPP
#define GRNODE_HPP

#include <sh/ShProgram.hpp>
#include <map>
#include <list>

namespace OGLFT {
  class Face;
};

class GrPort;
class GrView;

class GrNode {
public:
  GrNode(const SH::ShProgram& program,
         double x, double y,
         OGLFT::Face* face,
         GrView* view);

  ~GrNode();
  
  void draw_box();
  void draw_edges(); // Draws outgoing edges only.
  void draw_labels();

  void combine(const SH::ShProgram& addon);
  
  void moveTo(double x, double y);

  double x() const { return m_x; }
  double y() const { return m_y; }

  GrView* view() const { return m_view; }

  typedef std::vector<GrPort*> PortList;

  PortList::iterator inputs_begin() { return m_input_ports.begin(); }
  PortList::iterator inputs_end() { return m_input_ports.end(); }
  PortList::iterator outputs_begin() { return m_output_ports.begin(); }
  PortList::iterator outputs_end() { return m_output_ports.end(); }
  
  void clearMarked();
  void mark(bool marked) { m_marked = marked; }
  bool marked() const { return m_marked; }

  SH::ShProgram program() { return m_program; }
  
  void calcSizes();

  double width() const { return m_width; }
  double height() const;

  unsigned int pickid() const { return m_gl_name; }
  
private:
  
  SH::ShProgram m_program;
  double m_x, m_y;
  OGLFT::Face* m_face;
  GrView* m_view;
  double m_width, m_height;
  double m_in_height, m_out_height;
  double m_in_width, m_out_width;

  double m_font_height;

  unsigned int m_gl_name;

  PortList m_input_ports, m_output_ports;

  bool m_marked;
};

#endif
