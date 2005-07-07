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
#ifndef SHRIKEFRAME_HPP
#define SHRIKEFRAME_HPP

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/minifram.h>

#include "UniformPanel.hpp"
#include "Shader.hpp"
#include "ShrikePropsDialog.hpp"

enum ShrikeId {
  SHRIKE_NIL, // Need this here to avoid an ID being 0, which causes
              // problems at least on Mac OS X.
  SHRIKE_MENU_OPEN_MODEL,
  SHRIKE_MENU_QUIT,
  
  SHRIKE_MENU_SHADER_PROPS,
  
  SHRIKE_MENU_SHADER_SHOW_VSH,
  SHRIKE_MENU_SHADER_SHOW_FSH,
  SHRIKE_MENU_SHADER_SHOW_VSHIF,
  SHRIKE_MENU_SHADER_SHOW_FSHIF,
  SHRIKE_MENU_SHADER_SHOW_VSHIR,
  SHRIKE_MENU_SHADER_SHOW_FSHIR,

  SHRIKE_MENU_SHADER_OPTS,
  SHRIKE_MENU_SHADER_OPTS_LIFTING,
  SHRIKE_MENU_SHADER_OPTS_PROPAGATION,
  SHRIKE_MENU_SHADER_OPTS_DEADCODE,
  SHRIKE_MENU_SHADER_OPTS_SUBST,
  SHRIKE_MENU_SHADER_OPTS_COPY,
  SHRIKE_MENU_SHADER_OPTS_STRAIGHT,

  SHRIKE_MENU_SHADER_REINIT,

  SHRIKE_MENU_SHADER_OPTIMIZE,

  SHRIKE_MENU_VIEW_RESET,
  SHRIKE_MENU_VIEW_SCREENSHOT,
  SHRIKE_MENU_VIEW_BACKGROUND,
  SHRIKE_MENU_VIEW_FULLSCREEN,
  SHRIKE_MENU_VIEW_FPS,
  SHRIKE_MENU_VIEW_WIREFRAME,

  SHRIKE_TREECTRL_SHADERS
};

class ShrikeCanvas;
class wxSplitterWindow;

class ShrikeFrame : public wxFrame {
public:
  ShrikeFrame();
  virtual ~ShrikeFrame();

  void quit(wxCommandEvent& event);
  void openModel(wxCommandEvent& event);

  void close(wxCloseEvent& event);

  void onShaderSelect(wxTreeEvent& event);
  
  bool setShader(Shader* shader);

  void shaderProps(wxCommandEvent& event);
  void showVsh(wxCommandEvent& event);
  void showFsh(wxCommandEvent& event);
  void showVshIr(wxCommandEvent& event);
  void showFshIr(wxCommandEvent& event);
  void showVshInterface(wxCommandEvent& event);
  void showFshInterface(wxCommandEvent& event);
  void reinit(wxCommandEvent& event);

  void resetView(wxCommandEvent& event);
  void setBackground(wxCommandEvent& event);
  void fullscreen(wxCommandEvent& event);
  void optimize(wxCommandEvent& event);
  void setopts(wxCommandEvent& event);
  void wireframe(wxCommandEvent& event);
  void screenshot(wxCommandEvent& event);
  void setFullscreen(bool);
  void fps(wxCommandEvent& event);
  void setFps(bool);

  void keyDown(wxKeyEvent& event);

  static ShrikeFrame* instance();
  
  void showIR(SH::ShProgram program,
              const wxString& name);

  void showError(const wxString& message,
                 const std::string& details = ""); // TODO: Make this part unicode aware as well?
  
private:
  void showProgram(SH::ShProgram program,
                   const wxString& name);
  void showInterface(SH::ShProgram program,
                     const wxString& name);

  wxTreeCtrl* initShaderList(wxWindow* parent);

  ShrikeCanvas* m_canvas;
  UniformPanel* m_panel;
  wxFrame* m_preview;
  wxSplitterWindow* m_hsplitter;
  wxSplitterWindow* m_right_window;

  wxTreeCtrl* m_shaderList;
  Shader* m_shader;

  wxMenu* m_shaderMenu;
  wxMenu* m_viewMenu;
  
  bool m_fullscreen;
  bool m_fps;

  static ShrikeFrame* m_instance;
  DECLARE_EVENT_TABLE()
};

#endif
