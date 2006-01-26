// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////
#ifndef SHRIKECANVAS_HPP
#define SHRIKECANVAS_HPP

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <shutil/ShObjMesh.hpp>
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
