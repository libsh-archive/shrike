#ifndef SHTUDIO_HPP
#define SHTUDIO_HPP

#include <wx/glcanvas.h>
#include <vector>
#include <sh/ShProgram.hpp>

namespace OGLFT {
  class Face;
};

class GrNode;
class GrPort;
class GrMonitor;

enum PickType {
  PICK_NODE,
  PICK_PORT,
  PICK_EDGE
};

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

  void keydown(wxKeyEvent& event);
  
  GrNode* addProgram(const SH::ShProgram& program, int x, int y);

  void addMonitor(GrMonitor* monitor);
  
  int addPicker(PickType type, void* data);
  int pick(int x, int y);

  void layout();
  
private:
  void init();
  void setupView();

  bool m_init;

  double m_zoom;
  double tx, ty;

  long m_last_x, m_last_y;
  
  OGLFT::Face* m_font;

  struct PickInfo {
    int id;
    PickType type;
    void* data;
  };
  typedef std::vector<PickInfo> PickList;
  PickList m_pickables;
  
  typedef std::vector<GrNode*> NodeList;
  NodeList m_nodes;

  typedef std::list<GrMonitor*> MonitorList;
  MonitorList m_monitors;

  GrNode* m_selected;
  GrPort* m_connecting;
  double m_sel_dx, m_sel_dy;

  struct {
    double x_from, y_from, x_to, y_to;
  } m_current_edge;

  GrNode* m_inputs_vtx;
  GrNode* m_inbetween;
  GrNode* m_outputs_frg;
  
  DECLARE_EVENT_TABLE()
};

#endif
