#ifndef SHRIKECANVAS_HPP
#define SHRIKECANVAS_HPP

#include <wx/glcanvas.h>
#include <sh/ShObjMesh.hpp>
#include "Camera.hpp"
#include "Shader.hpp"

class ShrikeCanvas : public wxGLCanvas {
public:
  ShrikeCanvas(wxWindow* parent,
               ShUtil::ShObjMesh* model);
  
  void render();
  void renderObject();
  
  void setModel(ShUtil::ShObjMesh* model);
  const ShUtil::ShObjMesh* getModel() const;
  
  void paint();
  void reshape();
  void motion(wxMouseEvent& event);

  void resetView();

  void setBackground(unsigned char r, unsigned char g, unsigned char b);
  
  void setShader(Shader* shader);
  
  static ShrikeCanvas* instance();
  
private:
  void init();
  void setupView();
  
  bool m_init;
  ShUtil::ShObjMesh* m_model;
  Camera m_camera;

  long m_last_x, m_last_y;

  Shader* m_shader;

  bool m_showLight;

  float m_bg_r;
  float m_bg_g;
  float m_bg_b;
  
  static ShrikeCanvas* m_instance;
  
  DECLARE_EVENT_TABLE()
};

#endif
