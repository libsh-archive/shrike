#ifndef SHTUDIO_HPP
#define SHTUDIO_HPP

#include <wx/glcanvas.h>
#include <list>
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

  void addProgram(const SH::ShProgram& program, int x, int y);
  
private:
  void init();
  void setupView();

  bool m_init;

  double m_zoom;
  double tx, ty;

  long m_last_x, m_last_y;
  
  OGLFT::Face* m_font;

  std::list<GrNode*> m_nodes;

  DECLARE_EVENT_TABLE()
};

#endif
