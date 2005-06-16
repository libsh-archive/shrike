// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#ifndef SHRIKECANVAS_HPP
#define SHRIKECANVAS_HPP

#include <wx/wx.h>
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
  
  void paint(wxPaintEvent& event);
  void reshape(wxSizeEvent& event);
  void motion(wxMouseEvent& event);

  void screenshot(const wxString& filename);  

  void resetView();

  void setBackground(unsigned char r, unsigned char g, unsigned char b);
  
  void setShader(Shader* shader);

  void setShowFps(bool);

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

  SH::ShAttrib1f m_fps;
  SH::ShProgram m_fpsVsh;
  SH::ShProgram m_fpsFsh;
  bool m_showFps;
  SH::ShProgramSet* m_fps_shaders;

  float m_bg_r;
  float m_bg_g;
  float m_bg_b;
  SH::ShColor3f m_bg;

  
  static ShrikeCanvas* m_instance;
  
  DECLARE_EVENT_TABLE()
};

#endif
