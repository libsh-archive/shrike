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

#include "Project.hpp"
#include "Shader.hpp"
#include "ShrikePropsDialog.hpp"
#include "UniformPanel.hpp"

enum ShrikeId {
  SHRIKE_NIL, // Need this here to avoid an ID being 0, which causes
              // problems at least on Mac OS X.
  SHRIKE_MENU_OPEN_MODEL,
  SHRIKE_MENU_QUIT,
  
  SHRIKE_MENU_PROJECT_NEW,
  SHRIKE_MENU_PROJECT_OPEN,
  SHRIKE_MENU_PROJECT_SAVE,
  SHRIKE_MENU_PROJECT_CLOSE,
  SHRIKE_MENU_PROJECT_NEW_SRC,
  SHRIKE_MENU_PROJECT_ADD_SRC,
  SHRIKE_MENU_PROJECT_BUILD,
  SHRIKE_MENU_PROJECT_BUILD_SETTINGS,

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

  SHRIKE_TREECTRL_SHADERS,
  SHRIKE_TREECTRL_PROJECTS
};

class ProjectMenu;
class ShaderMenu;
class ShUtil::ShObjMesh;
class ShrikeCanvas;
class wxSplitterWindow;

class ShrikeFrame : public wxFrame {
public:
  ShrikeFrame();
  virtual ~ShrikeFrame();

  bool set_shader(Shader* shader);
  Shader* get_shader() { return m_shader; }

  wxListBox* output() { return m_output; }

  void show_program(SH::ShProgram program, const wxString& name);
  void show_interface(SH::ShProgram program, const wxString& name);
  void show_ir(SH::ShProgram program, const wxString& name);

  void show_error(const wxString& message,
                 const std::string& details = ""); // TODO: Make this part unicode aware as well?
  
  Project* get_project() { return m_project; }
  void set_project(Project* project);

  static ShrikeFrame* instance();
private:
  ShUtil::ShObjMesh* init_model();
  wxTreeCtrl* init_shader_list(wxWindow* parent);

  void on_quit(wxCommandEvent& event);
  void on_open_model(wxCommandEvent& event);
  void on_close(wxCloseEvent& event);
  void on_keydown(wxKeyEvent& event);
  void on_shader_item_select(wxTreeEvent& event);
  void on_shader_item_right_click(wxTreeEvent& event);
  
  void on_reset_view(wxCommandEvent& event);
  void on_set_background(wxCommandEvent& event);
  void on_fullscreen(wxCommandEvent& event);
  void on_wireframe(wxCommandEvent& event);
  void on_screenshot(wxCommandEvent& event);
  void on_fps(wxCommandEvent& event);

  void set_fullscreen(bool);
  void set_fps(bool);
  
  void on_project_new(wxCommandEvent& event);
  void on_project_open(wxCommandEvent& event);
  void on_project_save(wxCommandEvent& event);
  void on_project_close(wxCommandEvent& event);
  void on_project_new_source(wxCommandEvent& event);
  void on_project_add_source(wxCommandEvent& event);
  void on_project_build(wxCommandEvent& event);
  void on_project_build_settings(wxCommandEvent& event);
  void on_project_item_select(wxTreeEvent& event);
  void on_project_item_activated(wxTreeEvent& event);
  void on_project_item_right_click(wxTreeEvent& event);

  ShrikeCanvas* m_canvas;
  UniformPanel* m_panel;
  wxFrame* m_preview;
  wxListBox* m_output;

  wxTreeCtrl* m_project_tree;
  wxTreeCtrl* m_shaderList;
  Shader* m_shader;

  ProjectMenu* m_project_menu;
  ShaderMenu* m_shader_menu;
  wxMenu* m_viewMenu;
  
  Project* m_project;

  bool m_fullscreen;
  bool m_fps;

  static ShrikeFrame* m_instance;
  DECLARE_EVENT_TABLE()
};

#endif
