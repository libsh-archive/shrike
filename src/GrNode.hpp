#ifndef GRNODE_HPP
#define GRNODE_HPP

#include <sh/ShProgram.hpp>
#include <map>
#include <list>

namespace OGLFT {
  class Face;
};

class GrNode {
public:
  GrNode(const SH::ShProgram& program,
         double x, double y,
         OGLFT::Face* face);
  
  void draw_box();
  void draw_edges(); // Draws outgoing edges only.
  void draw_labels();

  void moveTo(double x, double y);

  double x() const { return m_x; }
  double y() const { return m_y; }
  
private:
  
  SH::ShProgram m_program;
  double m_x, m_y;
  OGLFT::Face* m_face;
  double m_width, m_height;
  double m_in_height, m_out_height;
  double m_in_width, m_out_width;

  double m_font_height;

  unsigned int m_gl_name;

  static unsigned int m_max_gl_name;

  struct VarDest {
    SH::ShProgram program;
    SH::ShVariableNodePtr var;
    bool outgoing;
  };
  
  typedef std::map<SH::ShVariableNodePtr, std::list<VarDest> > VarMap;

  VarMap m_outgoing;
  VarMap m_incoming;
};

#endif
