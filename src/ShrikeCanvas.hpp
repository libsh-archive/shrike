#ifndef SHRIKECANVAS_HPP
#define SHRIKECANVAS_HPP

#include <wx/glcanvas.h>
#include "ShObjFile.hpp"
#include "Camera.hpp"

class ShrikeCanvas : public wxGLCanvas {
public:
  ShrikeCanvas(wxWindow* parent,
               SH::ShObjFile* model);
  
  void render();

  void setModel(SH::ShObjFile* model);
  
  void paint();
  void reshape();
  void motion(wxMouseEvent& event);

  static ShrikeCanvas* instance();
  
private:
  void init();
  void setupView();
  
  bool m_init;
  SH::ShObjFile* m_model;
  Camera m_camera;

  long m_last_x, m_last_y;

  static ShrikeCanvas* m_instance;
  
  DECLARE_EVENT_TABLE()
};

#endif
