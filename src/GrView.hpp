#ifndef SHTUDIO_HPP
#define SHTUDIO_HPP

#include <wx/glcanvas.h>
#include <vector>
#include <sh/ShProgram.hpp>

namespace OGLFT {
  class Face;
};

class GrNode;

class GrView : public wxGLCanvas {
public:
  GrView(wxWindow* parent);
  ~GrView();
  void paint();
  void reshape();
  void motion(wxMouseEvent& event);
  void mousewheel(wxMouseEvent& event);
  void mdown(wxMouseEvent& event);
  void mup(wxMouseEvent& event);

  void addProgram(const SH::ShProgram& program, int x, int y);
  
private:
  void init();
  void setupView();

  bool m_init;

  double m_zoom;
  double tx, ty;

  long m_last_x, m_last_y;
  
  OGLFT::Face* m_font;
  
  typedef std::vector<GrNode*> NodeList;
  NodeList m_nodes;

  GrNode* m_selected;
  double m_sel_dx, m_sel_dy;

  DECLARE_EVENT_TABLE()
};

#endif
