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

  void screenshot(const std::string& filename);
  
  void resetView();

  void setBackground(unsigned char r, unsigned char g, unsigned char b);
  
  void setShader(Shader* shader);

  void keyDown(wxKeyEvent& event);
  
  static ShrikeCanvas* instance();
  
private:
  void init();
  void setupView(int split = 1, int x = 0, int y = 0);
  
  bool m_init;
  ShUtil::ShObjMesh* m_model;
  bool m_model_dirty; // whether to regenerate display list on next render
  unsigned int m_model_list; // OpenGl display list


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
