#ifndef GRNODE_HPP
#define GRNODE_HPP

#include <sh/ShProgram.hpp>

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
  
private:

  SH::ShProgram m_program;
  double m_x, m_y;
  OGLFT::Face* m_face;
  double m_width, m_height;
  double m_in_height, m_out_height;
  double m_in_width, m_out_width;

  double m_font_height;
};

#endif
