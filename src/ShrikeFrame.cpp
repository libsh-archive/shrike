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
#include <iostream>
#include <fstream>
#include <shutil/ShObjMesh.hpp>
#include <wx/colordlg.h>
#include <wx/config.h>
#include <wx/propdlg.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include "AboutDialog.hpp"
#include "Build.hpp"
#include "Globals.hpp"
#include "Project.hpp"
#include "Shader.hpp"
#include "ShrikeCanvas.hpp"
#include "ShrikeFrame.hpp"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace SH;
using namespace ShUtil;

BEGIN_EVENT_TABLE(ShrikeFrame, wxFrame)
  EVT_MENU(SHRIKE_MENU_OPEN_MODEL, ShrikeFrame::on_open_model)
  EVT_MENU(SHRIKE_MENU_QUIT, ShrikeFrame::on_quit)

  EVT_MENU(SHRIKE_MENU_PROJECT_NEW, ShrikeFrame::on_project_new)
  EVT_MENU(SHRIKE_MENU_PROJECT_OPEN, ShrikeFrame::on_project_open)
  EVT_MENU(SHRIKE_MENU_PROJECT_SAVE, ShrikeFrame::on_project_save)
  EVT_MENU(SHRIKE_MENU_PROJECT_CLOSE, ShrikeFrame::on_project_close)
  EVT_MENU(SHRIKE_MENU_PROJECT_NEW_SRC, ShrikeFrame::on_project_new_source)
  EVT_MENU(SHRIKE_MENU_PROJECT_ADD_SRC, ShrikeFrame::on_project_add_source)
  EVT_MENU(SHRIKE_MENU_PROJECT_BUILD_SETTINGS, ShrikeFrame::on_project_build_settings)
  EVT_MENU(SHRIKE_MENU_PROJECT_BUILD, ShrikeFrame::on_project_build)
  EVT_TREE_SEL_CHANGED(SHRIKE_TREECTRL_PROJECTS, ShrikeFrame::on_project_item_select)
  EVT_TREE_ITEM_ACTIVATED(SHRIKE_TREECTRL_PROJECTS, ShrikeFrame::on_project_item_activated)
  EVT_TREE_ITEM_RIGHT_CLICK(SHRIKE_TREECTRL_PROJECTS, ShrikeFrame::on_project_item_right_click)

  EVT_MENU(SHRIKE_MENU_VIEW_RESET, ShrikeFrame::on_reset_view)
  EVT_MENU(SHRIKE_MENU_VIEW_SCREENSHOT, ShrikeFrame::on_screenshot)
  EVT_MENU(SHRIKE_MENU_VIEW_BACKGROUND, ShrikeFrame::on_set_background)
  EVT_MENU(SHRIKE_MENU_VIEW_FULLSCREEN, ShrikeFrame::on_fullscreen)
  EVT_MENU(SHRIKE_MENU_VIEW_WIREFRAME, ShrikeFrame::on_wireframe)
  EVT_MENU(SHRIKE_MENU_VIEW_FPS, ShrikeFrame::on_fps)

  EVT_MENU(SHRIKE_MENU_HELP_ABOUT, ShrikeFrame::on_about)

  EVT_CLOSE(ShrikeFrame::on_close)
  EVT_KEY_DOWN(ShrikeFrame::on_keydown)

  EVT_TREE_SEL_CHANGED(SHRIKE_TREECTRL_SHADERS, ShrikeFrame::on_shader_item_select)
  EVT_TREE_ITEM_RIGHT_CLICK(SHRIKE_TREECTRL_SHADERS, ShrikeFrame::on_shader_item_right_click)
END_EVENT_TABLE()

struct ShaderTreeData : public wxTreeItemData {
  ShaderTreeData(Shader* shader)
    : shader(shader)
  {
  }
  
  Shader* shader;
};

class ProjectMenu : public wxMenu
{
public:
  ProjectMenu(const wxString& title=wxT(""), long style=0)
    : wxMenu(title, style)
  {
    Append(SHRIKE_MENU_PROJECT_NEW, wxT("&New..."));
    Append(SHRIKE_MENU_PROJECT_OPEN, wxT("&Open..."));
    Append(SHRIKE_MENU_PROJECT_SAVE, wxT("&Save"));
    Append(SHRIKE_MENU_PROJECT_CLOSE, wxT("&Close"));
    AppendSeparator();
    Append(SHRIKE_MENU_PROJECT_NEW_SRC, wxT("New source file..."));
    Append(SHRIKE_MENU_PROJECT_ADD_SRC, wxT("Add source file..."));
    AppendSeparator();
    Append(SHRIKE_MENU_PROJECT_BUILD_SETTINGS, wxT("Build Settings"));
    Append(SHRIKE_MENU_PROJECT_BUILD, wxT("&Build"));

    enable(false);
  }

  void enable(bool project)
  {
    Enable(SHRIKE_MENU_PROJECT_SAVE, project);
    Enable(SHRIKE_MENU_PROJECT_CLOSE, project);
    Enable(SHRIKE_MENU_PROJECT_NEW_SRC, project);
    Enable(SHRIKE_MENU_PROJECT_ADD_SRC, project);
    Enable(SHRIKE_MENU_PROJECT_BUILD, project);
  }
};

class ShaderMenu : public wxMenu
{
public:
  ShaderMenu(ShrikeFrame* frame, const wxString& title=wxT(""), long style=0)
    : wxMenu(title, style), m_frame(frame)
  {
    Append(SHRIKE_MENU_SHADER_PROPS, wxT("&Properties") );
    AppendSeparator();
    Append(SHRIKE_MENU_SHADER_REINIT, wxT("Re&initialize") );
    AppendSeparator();
    Append(SHRIKE_MENU_SHADER_SHOW_VSHIF, wxT("Show &vertex interface") );
    Append(SHRIKE_MENU_SHADER_SHOW_FSHIF, wxT("Show &fragment interface") );
    AppendSeparator();
    Append(SHRIKE_MENU_SHADER_SHOW_VSH, wxT("Show &vertex assembly") );
    Append(SHRIKE_MENU_SHADER_SHOW_FSH, wxT("Show &fragment assembly") );
    AppendSeparator();
    Append(SHRIKE_MENU_SHADER_SHOW_VSHIR, wxT("Show &vertex IR") );
    Append(SHRIKE_MENU_SHADER_SHOW_FSHIR, wxT("Show &fragment IR") );
    AppendSeparator();
    AppendCheckItem(SHRIKE_MENU_SHADER_OPTIMIZE, wxT("Turn on &optimizations"));
    Check(SHRIKE_MENU_SHADER_OPTIMIZE, true);

    m_opts = new wxMenu();
    Append(SHRIKE_MENU_SHADER_OPTS, wxT("Optimizations"), m_opts);

    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_LIFTING, wxT("Uniform Lifting"));
    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_PROPAGATION, wxT("Constant/Uniform Propagation"));
    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_DEADCODE, wxT("Dead Code Removal"));
    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_SUBST, wxT("Forward Substitution"));
    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_COPY, wxT("Copy Propagation"));
    m_opts->AppendCheckItem(SHRIKE_MENU_SHADER_OPTS_STRAIGHT, wxT("Straightening"));
    for (size_t i = 0; i < m_opts->GetMenuItemCount(); ++i)
      m_opts->FindItemByPosition(i)->Check(true);
  }

private:
  void on_properties(wxCommandEvent& event)
  {
    Shader *shader = m_frame->get_shader();
    if (!shader || shader->paramCount() == 0) 
      return;

    ShrikePropsDialog dialog(m_frame, shader);
    if (dialog.ShowModal() != wxID_OK)
      return;

    try {
      shader->init();
    } catch (const ShException& e) {
      std::cerr << e.message() << std::endl;
      return;
    } catch (...) {
      std::cerr << "Unknown exception caught!" << std::endl;
      return;
    }
    m_frame->set_shader(shader);
  }

  void on_optimize(wxCommandEvent& event)
  {
    if (event.IsChecked()) {
      ShContext::current()->optimization(2);
    } else {
      ShContext::current()->optimization(0);
    }
    for (size_t i = 0; i < m_opts->GetMenuItemCount(); ++i)
      m_opts->FindItemByPosition(i)->Enable(event.IsChecked());
  }

  void on_optimize_item(wxCommandEvent& event)
  {
    std::string name;
    switch (event.GetId()) {
      case SHRIKE_MENU_SHADER_OPTS_LIFTING: name = "uniform lifting"; break;
      case SHRIKE_MENU_SHADER_OPTS_PROPAGATION: name = "propagation"; break;
      case SHRIKE_MENU_SHADER_OPTS_DEADCODE: name = "deadcode"; break;
      case SHRIKE_MENU_SHADER_OPTS_SUBST: name = "forward substitution"; break;
      case SHRIKE_MENU_SHADER_OPTS_COPY: name = "copy propagation"; break;
      case SHRIKE_MENU_SHADER_OPTS_STRAIGHT: name = "straightening"; break;
      default: return;
    }
    if (event.IsChecked()) {
      ShContext::current()->enable_optimization(name);
    } else {
      ShContext::current()->disable_optimization(name);
    }
  }
  
  void on_reinit(wxCommandEvent& event)
  {
    if (!m_frame->get_shader()) return;
    try {
      m_frame->get_shader()->init();
    } catch (const ShException& e) {
      std::cerr << e.message() << std::endl;
      return;
    }
    m_frame->set_shader(m_frame->get_shader());
  }

  void on_show_vsh(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_program(m_frame->get_shader()->vertex(), wxT("Vertex") );
  }

  void on_show_fsh(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_program(m_frame->get_shader()->fragment(), wxT("Fragment") );
  }

  void on_show_vsh_ir(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_ir(m_frame->get_shader()->vertex(), wxT("Vertex") );
  }

  void on_show_fsh_ir(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_ir(m_frame->get_shader()->fragment(), wxT("Fragment") );
  }

  void on_show_vsh_interface(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_interface(m_frame->get_shader()->vertex(), wxT("Vertex") );
  }

  void on_show_fsh_interface(wxCommandEvent& event)
  {
    if (m_frame->get_shader()) 
      m_frame->show_interface(m_frame->get_shader()->fragment(), wxT("Fragment") );
  }

  ShrikeFrame* m_frame;
  wxMenu* m_opts;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ShaderMenu, wxMenu)
  EVT_MENU(SHRIKE_MENU_SHADER_PROPS, ShaderMenu::on_properties)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSH, ShaderMenu::on_show_vsh)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSH, ShaderMenu::on_show_fsh)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSHIR, ShaderMenu::on_show_vsh_ir)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSHIR, ShaderMenu::on_show_fsh_ir)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSHIF, ShaderMenu::on_show_vsh_interface)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSHIF, ShaderMenu::on_show_fsh_interface)
  EVT_MENU(SHRIKE_MENU_SHADER_REINIT, ShaderMenu::on_reinit)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTIMIZE, ShaderMenu::on_optimize)

  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_LIFTING, ShaderMenu::on_optimize_item)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_PROPAGATION, ShaderMenu::on_optimize_item)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_DEADCODE, ShaderMenu::on_optimize_item)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_SUBST, ShaderMenu::on_optimize_item)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_COPY, ShaderMenu::on_optimize_item)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_STRAIGHT, ShaderMenu::on_optimize_item)
END_EVENT_TABLE()

ShrikeFrame::ShrikeFrame()
  : wxFrame(0, -1, wxT("Shrike"), wxDefaultPosition, wxSize(600, 400)),
    m_shader(0), m_project(0), m_fullscreen(false), m_fps(false)
{
  m_instance = this;
  CreateStatusBar();

  GetStatusBar()->SetStatusText(wxT("Hold down shift to rotate the light instead of the camera."));
  
  // Setup menus
  wxMenu* fileMenu = new wxMenu();
  fileMenu->Append(SHRIKE_MENU_OPEN_MODEL, wxT("&Open Model...") );
  fileMenu->AppendSeparator();
  fileMenu->Append(SHRIKE_MENU_QUIT, wxT("&Quit") );

  m_project_menu = new ProjectMenu();
  m_shader_menu = new ShaderMenu(this);
  PushEventHandler(m_shader_menu);

  m_viewMenu = new wxMenu();
  m_viewMenu->Append(SHRIKE_MENU_VIEW_RESET, wxT("&Reset") );
  m_viewMenu->Append(SHRIKE_MENU_VIEW_BACKGROUND, wxT("Set &background colour...") );
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FULLSCREEN,_( "&Fullscreen") );
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_WIREFRAME, wxT("&Wireframe") );
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FPS, wxT("Show framera&te") );
  m_viewMenu->Append(SHRIKE_MENU_VIEW_SCREENSHOT, wxT("&Screenshot...") );

  wxMenu* help = new wxMenu();
  help->Append(SHRIKE_MENU_HELP_ABOUT, wxT("&About") );
  
  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, wxT("&File") );
  menuBar->Append(m_project_menu, wxT("&Project") );
  menuBar->Append(m_shader_menu, wxT("&Shader") );
  menuBar->Append(m_viewMenu, wxT("&View") );
  menuBar->Append(help, wxT("&Help") );
  
  SetMenuBar(menuBar);

#if 0 // switch this on for fixed size separate preview window
  // TODO implement this properly in the GUI
  int width = 512 + 2; 
  int height = 512 + 2; 
  m_preview = new wxFrame(0, -1, "Shrike Preview", wxDefaultPosition, wxSize(width, height));
  m_canvas = new ShrikeCanvas(m_preview, model);
  m_panel = new UniformPanel(m_hsplitter);
  m_hsplitter->SplitVertically(m_shaderList, m_panel, 150);
  m_preview->Show();

#else
  // This is the static layout. Layout will be made dynamic later
  // +---------+---------+--------+
  // |         |         | shader |
  // | shaders | canvas  | props  |
  // |         |         |        |
  // |---------|         |        |
  // |         |---------|        |
  // | project |         |        |
  // |         | output  |        |
  // +---------+---------+--------+
  wxSplitterWindow* col1_col23 = new wxSplitterWindow(this, -1);
  wxSplitterWindow* shaders_projects = new wxSplitterWindow(col1_col23, -1);
  wxSplitterWindow* col2_col3 = new wxSplitterWindow(col1_col23, -1);
  wxSplitterWindow* canvas_output = new wxSplitterWindow(col2_col3, -1);

  m_shaderList = init_shader_list(shaders_projects);
  m_project_tree = init_project_tree(shaders_projects); 
  m_panel = new UniformPanel(col2_col3);

  ShObjMesh* model = init_model();
  m_canvas = new ShrikeCanvas(canvas_output, model);
  m_output = new wxListBox(canvas_output,-1);
  
  col1_col23->SplitVertically(shaders_projects, col2_col3);
  col2_col3->SplitVertically(canvas_output, m_panel);
  canvas_output->SplitHorizontally(m_canvas, m_output);
  shaders_projects->SplitHorizontally(m_shaderList, m_project_tree);

#if wxMAJOR_VERSION==2 && wxMINOR_VERSION==6
    /*empty*/
#else
  col2_col3->SetSashGravity(1.0);
  canvas_output->SetSashGravity(1.0);
#endif
  canvas_output->SetMinimumPaneSize(40);
//  m_right_window->SetMinimumPaneSize(40);

  col1_col23->SetSashPosition(200);
  col2_col3->SetSashPosition(200);
#endif
}

ShrikeFrame::~ShrikeFrame()
{
  PopEventHandler();
}

void ShrikeFrame::set_project(Project* project)
{
  m_project = project;
  m_project_menu->enable(m_project != 0);
  if (project) {
    SetTitle(wxT("Shrike [")+project->config()+wxT("]"));
  }
  else {
    SetTitle(wxT("Shrike"));
  }
}

ShObjMesh* ShrikeFrame::init_model()
{
  ShObjMesh* model=0;
  std::ifstream infile(SHMEDIA_DIR "/objs/plane1.obj");
  if (infile) {
    model = new ShObjMesh(infile);
  }
  else {
    show_error(
      wxT("The shmedia package was not found. This package contains models and textures \n"
          "required by many shaders. You may still run Shrike but it is recommended \n"
          "you install the shmedia package first.\n"
          "\n"
	        "The package can be found at http://libsh.org/ in the downloads section.\n"
          "\n"
	        "Shrike expected the package in "SHMEDIA_DIR),
      "failed to open " SHMEDIA_DIR "/objs/plane1.obj"
      );
    model = new ShObjMesh();
    model->clear();
    std::ostringstream oss;
    oss << "v 1.0 1.0 0.0" << std::endl;
    oss << "v 1.0 -1.0 0.0" << std::endl;
    oss << "v -1.0 -1.0 0.0" << std::endl;
    oss << "v -1.0 1.0 0.0" << std::endl;
    oss << "vt 1.0 0.0" << std::endl;
    oss << "vt 1.0 1.0" << std::endl;
    oss << "vt 0.0 1.0" << std::endl;
    oss << "vt 0.0 0.0" << std::endl;
    oss << "vn 0.0 0.0 1.0" << std::endl;
    oss << "f 1/1/1 2/2/1 3/3/1" << std::endl;
    oss << "f 1/1/1 3/3/1 4/4/1" << std::endl;
    std::istringstream iss(oss.str());
    model->readObj(iss);
  }
  return model;
}

void ShrikeFrame::on_open_model(wxCommandEvent& event)
{
  wxFileDialog dialog(this, wxT("Open Model"),
                      SHMEDIA_DIR wxT("/objs"), wxT(""),
                      wxT("OBJ Files (*.obj)|*.obj"), wxOPEN);

  if (dialog.ShowModal() == wxID_OK) {
    std::ifstream infile(dialog.GetPath().fn_str());
    if (infile) {
      try {
        ShObjMesh* model = new ShObjMesh(infile);
        m_canvas->setModel(model);
      }
      catch (const ShException& e) {
        show_error(wxT("The model ") + dialog.GetPath() + wxT(" failed to load"),
                   e.message());
      }
    }
  }
}

void ShrikeFrame::on_quit(wxCommandEvent& event)
{
  Close(true);
}

void ShrikeFrame::on_close(wxCloseEvent& event)
{
  Destroy();
}

void ShrikeFrame::on_shader_item_select(wxTreeEvent& event)
{
  wxTreeItemId item = event.GetItem();
  
  if (ShaderTreeData* data = dynamic_cast<ShaderTreeData*>(m_shaderList->GetItemData(item))) {
    if (!data) return;
    Shader* shader = data->shader;
    if (!set_shader(shader)) {
      m_shaderList->SetItemTextColour(item, *wxRED);
    }
  }
}

void ShrikeFrame::on_shader_item_right_click(wxTreeEvent& event)
{
  PopupMenu(m_shader_menu, event.GetPoint());
}

bool ShrikeFrame::set_shader(Shader* shader)
{
  if (shader && shader->failed()) {
    // TODO Perhaps set some status bar thing here.
    // Can we change the colour of the list item belonging to the
    // shader? that would be cool.
    return false;
  }
  m_canvas->SetCurrent();
  try {
    if (shader) shader->firstTimeInit();
    if (shader) shader->bind();
  } catch (const ShImageException& e) {
    shader->set_failed(true);
    show_error(wxT("An Image error occured trying to initialize or bind this program.\n")
              wxT("This probably indicates a missing or corrupt texture image."),
              e.message());
    return false;
  } catch (const ShBackendException& e) {
    shader->set_failed(true);
    show_error(wxT("A Backend error occured trying to initialize or bind this program.\n")
              wxT("This probably indicates that your graphics card\n")
              wxT("does not have the resources required to run this program."),
              e.message());
    return false;
  } catch (const ShException& e) {
    shader->set_failed(true);
    show_error(wxT("An Sh error occured trying to initialize or bind this program.\n")
              wxT("This probably indicates an error in the shader program,\n")
              wxT("or an error trying to run the shader on your hardware\n")
              wxT("(e.g. running out of instructions). It may also be a missing\n")
              wxT("or corrupt texture image."),
              e.message());
    return false;
  } catch (...) {
    shader->set_failed(true);
    show_error(wxT("An Unknown error occured trying to initialize or bind this program.\n")
              wxT("This probably indicates an error in the shader program.") );
    return false;
  }
  m_canvas->setShader(shader);
  m_canvas->render();
  m_panel->setShader(shader);
  m_shader = shader;
  return true;
}

void ShrikeFrame::show_error(const wxString& message,
                            const std::string& details)
{
  if (!details.empty()) {
    wxLogWarning(wxConvLibc.cMB2WX(details.c_str()));
  }
  wxLogError(message.GetData());
  wxLog::FlushActive();
}

void ShrikeFrame::on_reset_view(wxCommandEvent& event)
{
  m_canvas->resetView();
}

void ShrikeFrame::on_set_background(wxCommandEvent& event)
{
  wxColourDialog dialog(this);

  if (dialog.ShowModal() != wxID_OK) return;

  wxColour c = dialog.GetColourData().GetColour();
  m_canvas->setBackground(c.Red(), c.Green(), c.Blue());
}

void ShrikeFrame::show_program(ShProgram program,
                              const wxString& name)
{
  if (!program.node()) return;
  if (!program.node()->code()) return;

  wxString title = name + wxT(" Shader Code");
  wxFrame* frame = new wxFrame(0, -1, title.c_str());


  wxTextCtrl* control = new wxTextCtrl(frame, -1, wxT(""),
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);

#ifndef NO_TEXT_WINDOW_STREAM
  std::ostream stream(control);
  program.node()->code()->print(stream);
#else
  std::ostringstream s;
  program.node()->code()->print(s);
  control->AppendText(wxString(s.str().c_str(), *wxConvCurrent));
#endif

  frame->Show();
}

void ShrikeFrame::show_interface(ShProgram program,
                                const wxString& name)
{
  if (!program.node()) return;
  if (!program.node()->code()) return;

  wxString title = name + wxT(" Shader Interface");
  wxFrame* frame = new wxFrame(0, -1, title.c_str());

  wxTextCtrl* control = new wxTextCtrl(frame, -1, wxT(""),
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);
  
  control->AppendText(wxConvLibc.cMB2WX(program.describe_interface().c_str()));

  frame->Show();
}

void ShrikeFrame::show_ir(ShProgram program,
                         const wxString& name)
{
  if (!program.node()) return;

  wxString title = name + wxT(" Shader Code");
  wxFrame* frame = new wxFrame(0, -1, title.c_str());


  wxTextCtrl* control = new wxTextCtrl(frame, -1, wxT(""),
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);

#ifndef NO_TEXT_WINDOW_STREAM
  std::ostream stream(control);
  program.node()->ctrlGraph->print(stream, 0);
#else
  std::ostringstream s;
  program.node()->ctrlGraph->print(s, 0);
  control->AppendText(wxString(s.str().c_str(), *wxConvCurrent));
#endif

  frame->Show();
}

void ShrikeFrame::on_fullscreen(wxCommandEvent& event)
{
  set_fullscreen(event.IsChecked());
}

void ShrikeFrame::on_wireframe(wxCommandEvent& event)
{
  m_canvas->SetCurrent();
  
  if (event.IsChecked()) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  m_canvas->Refresh(FALSE);
}

void ShrikeFrame::set_fullscreen(bool fs)
{
  ShowFullScreen(fs);
//  if (fs) {
//    m_hsplitter->Unsplit(m_shaderList);
//  } else {
//    if (!m_hsplitter->IsSplit()) {
//      m_hsplitter->SplitVertically(m_shaderList, m_right_window,
//                                   150);
//      m_shaderList->Show();
//    }
//  }
  m_viewMenu->Check(SHRIKE_MENU_VIEW_FULLSCREEN, fs);
  m_fullscreen = fs;
}

void ShrikeFrame::on_fps(wxCommandEvent& event)
{
  set_fps(event.IsChecked());
}

void ShrikeFrame::set_fps(bool fps)
{
  m_fps = fps;
  ShrikeCanvas::instance()->setShowFps(m_fps);
}

void ShrikeFrame::on_keydown(wxKeyEvent& event)
{
  if (event.GetKeyCode() == WXK_ESCAPE) {
    if (m_fullscreen) {
      set_fullscreen(false);
    }
  } else {
    event.Skip();
  }
}

void ShrikeFrame::on_screenshot(wxCommandEvent& event)
{
  wxFileDialog* dialog = new wxFileDialog(this, wxT("Save Screenshot"),
                                          wxT("."), wxT(""),
                                          wxT("PNG Files (*.png)|*.png"), wxSAVE);
  if (dialog->ShowModal() == wxID_OK) {
    m_canvas->screenshot(dialog->GetPath().c_str());
  }
}

void ShrikeFrame::on_about(wxCommandEvent& event)
{
  AboutDialog dialog(this);
  dialog.ShowModal();
}

ProjectTree* ShrikeFrame::init_project_tree(wxWindow* parent)
{
  ProjectTree* tree = new ProjectTree(parent, SHRIKE_TREECTRL_PROJECTS,
    wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | 
#ifndef WIN32
    wxTR_NO_LINES);
#else
    wxTR_LINES_AT_ROOT);
#endif
  tree->AddRoot(wxT(""));

  return tree;
}

wxTreeCtrl* ShrikeFrame::init_shader_list(wxWindow* parent)
{
#ifndef WIN32
  wxTreeCtrl* tree = new wxTreeCtrl(parent, SHRIKE_TREECTRL_SHADERS,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_NO_LINES);
#else
  wxTreeCtrl* tree = new wxTreeCtrl(parent, SHRIKE_TREECTRL_SHADERS,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT);
#endif

  wxTreeItemId root = tree->AddRoot( wxT("") );

  std::map<std::string, wxTreeItemId> nodes;
  
  for (ShaderList::iterator I = GetShaders().begin(); I != GetShaders().end(); ++I) {
    Shader* shader = *I;
    std::list<std::string> nodelist;
    std::string name = shader->name();
    while (1) {
      std::string::size_type i = name.find(": ");
      if (i == std::string::npos) {
        nodelist.push_back(name);
        break;
      } else {
        nodelist.push_back(name.substr(0, i));
        name = name.substr(i + 2);
      }
    }

    wxTreeItemId last = root;
    std::string fullname;
    for (std::list<std::string>::iterator N = nodelist.begin();
         N != nodelist.end(); ++N) {
      std::list<std::string>::iterator M = N;
      ++M;
      std::string name = *N;
      if (!fullname.empty()) fullname += ":";
      fullname += name;
      ShaderTreeData* data = 0;
      if (M == nodelist.end()) data = new ShaderTreeData(shader);
      if (nodes.find(fullname) == nodes.end()) {
        nodes[fullname] = tree->AppendItem(last, wxConvLibc.cMB2WX(name.c_str()), -1, -1,
                                           data);
      } else {
        if (data && tree->GetItemData(nodes[fullname]) == 0) {
          tree->SetItemData(nodes[fullname], data);
        }
      }
      last = nodes[fullname];
    }
  }

  tree->SortChildren(root);
  return tree;
}

void ShrikeFrame::on_project_item_select(wxTreeEvent& event)
{
  wxTreeItemId item = event.GetItem();
  Project* project = m_project_tree->get_project(item);

  if (!project) {
    set_project(0);
    return;
  }

  set_project(project);

  switch (m_project_tree->get_item_type(item)) {
    case ProjectTree::Shader: {
      if (!set_shader(m_project_tree->get_shader(item)))
        m_project_tree->SetItemTextColour(item, *wxRED);
      break;
    }
    case ProjectTree::Source: break;
    default: break;
  }
}

void ShrikeFrame::on_project_item_activated(wxTreeEvent& event)
{
  wxTreeItemId item = event.GetItem();
  Project* project = m_project_tree->get_project(item);

  if (!project) {
    set_project(0);
    return;
  }

  switch (m_project_tree->get_item_type(item)) {
    case ProjectTree::Shader: 
      break;
    case ProjectTree::Source: {
      wxConfig config(wxT("shrike"));
      wxString value;
      if (!config.Read(wxT("/Editor/Editor"), &value)) {
        // This is temporary until a preference panel is made
        wxFileDialog dialog(this, wxT("Choose your favourite C/C++ editor"), 
          wxT(""), wxT(""), wxT("*"));
        if (dialog.ShowModal() != wxID_OK)
          return;
        value = dialog.GetPath();
        config.Write(wxT("/Editor/Editor"), value);
      }
      wxFileName fname(project->workspace(), m_project_tree->get_source(item));
      wxExecute(value+wxT(" ")+fname.GetFullPath(), wxEXEC_ASYNC, 0);
      break;
    }
    default:
      break;
  }
}

void ShrikeFrame::on_project_item_right_click(wxTreeEvent& event)
{
  wxPoint a=event.GetPoint(), b=m_project_tree->GetPosition();
  wxPoint p(a.x+b.x, a.y+b.y);
  PopupMenu(m_project_menu, p);
}

void ShrikeFrame::on_project_new(wxCommandEvent& event)
{
  wxTextEntryDialog project(this, wxT("Enter project name"));
  if (project.ShowModal() != wxID_OK) return;
  wxDirDialog workspace(this, wxT("Select workspace directory"), wxFileName::GetCwd());
  if (workspace.ShowModal() != wxID_OK) return;
  wxTextEntryDialog target(this, wxT("Enter target name"));
  if (target.ShowModal() != wxID_OK) return;

  Project* p= new Project();
  p->name(project.GetValue());
  p->workspace(workspace.GetPath());
  p->config(project.GetValue()+wxT(".proj"));
  p->target(target.GetValue());
  m_project_tree->insert(p);
}

void ShrikeFrame::on_project_open(wxCommandEvent& event)
{
  wxFileDialog dialog(this, wxT("Choose a project file"), wxT(""), wxT(""), wxT("*.proj"));
  if (dialog.ShowModal() != wxID_OK)
    return;

  Project* project = new Project();
  if (!project->open(dialog.GetPath())) {
    delete project;
    return;
  }
  m_project_tree->insert(project);
}

void ShrikeFrame::on_project_save(wxCommandEvent& event)
{
  Project* project = m_project_tree->get_project(m_project_tree->GetSelection());
  if (project)
    project->save();
}

void ShrikeFrame::on_project_close(wxCommandEvent& event)
{
  Project* project = m_project_tree->get_project(m_project_tree->GetSelection());
  if (!project) 
    return;
  
  if (!project->saved()) {
    wxMessageDialog dialog(this, 
      wxT("Project ") + project->config() + wxT(" has unsaved changes.  Save now?"),
      wxT("Save changes?"), wxYES_NO|wxCANCEL|wxICON_QUESTION);
    int ret = dialog.ShowModal();
    if (ret == wxID_YES)
      project->save();
    else if (ret == wxID_CANCEL)
      return;
  }
  
  set_shader(0); 

  m_project_tree->remove();
}

void ShrikeFrame::on_project_new_source(wxCommandEvent& event)
{
  Project* project = m_project_tree->get_project(m_project_tree->GetSelection());
  if (!project) 
    return;

  wxTextEntryDialog dialog(this, wxT("Enter source file name"));
  if (dialog.ShowModal() != wxID_OK) return;
  
  wxFile file;
  wxFileName fname(project->workspace(), dialog.GetValue());
  if (!file.Create(fname.GetFullPath())) {
    return;
  }
  
  project->sources().push_back(dialog.GetValue()); 
  m_project_tree->update();
}

void ShrikeFrame::on_project_add_source(wxCommandEvent& event)
{
  Project* project = m_project_tree->get_project(m_project_tree->GetSelection());
  if (!project) return;

  wxFileDialog dialog(this, wxT("Choose a source file"), wxT(""), wxT(""), wxT("*.cpp"));
  if (dialog.ShowModal() != wxID_OK)
    return;
  
  wxFileName fname(dialog.GetPath());
  if (fname.GetPath() != project->workspace()) {
    wxMessageDialog msg(this, wxT("You can only add source files in the project workspace"));
    msg.ShowModal();
    return;
  }
  
  project->sources().push_back(fname.GetFullName()); 
  m_project_tree->update();
}

void ShrikeFrame::on_project_build(wxCommandEvent& event)
{
  Project* project = m_project_tree->get_project(m_project_tree->GetSelection());
  if (!project) return;

  output()->Clear();
  output()->Insert(wxT("Building ")+project->name()+wxT("..."),0);
  BuildProcess* process = build_project(*project);
  if (process->status() != 0) {
    wxInputStream *in = process->GetErrorStream();
    if (in) {
      wxTextInputStream text(*in);
      while (!in->Eof())
        output()->Insert(text.ReadLine(), output()->GetCount());
    }
    output()->Insert(wxT("Build failed"), output()->GetCount());
  }
  else {
    // see if the current shader belongs to the project just built
    if (get_shader()) {
      bool dirty = false;
      std::string old_shader=get_shader()->name();
      for (ShaderList::iterator I = project->begin_shaders(); I != project->end_shaders(); ++I) {
        if (get_shader() == *I) {
          dirty = true;
          break;
        }
      }
      project->load_shaders();
      if (dirty) {
        set_shader(0);
        for (ShaderList::iterator I = project->begin_shaders(); I != project->end_shaders(); ++I) {
          if ((*I)->name() == old_shader) {
            set_shader(*I);
            break;
          }
        }
      }
    }
    else {
      project->load_shaders();
    }
    m_project_tree->update();
    output()->Insert(wxT("Build successful"), output()->GetCount());
  }
  delete process;
}

void ShrikeFrame::on_project_build_settings(wxCommandEvent& event)
{
  wxPropertySheetDialog dialog(this, -1, wxT("Settings"));;
  dialog.CreateButtons(wxOK|wxCANCEL);
  dialog.SetSize(400,500);
  BuildPanel* panel = new BuildPanel(dialog.GetBookCtrl());
  dialog.GetBookCtrl()->AddPage(panel, wxT("Build"));
  if (dialog.ShowModal() == wxID_OK) {
    panel->save_config();
  }
}

ShrikeFrame* ShrikeFrame::instance()
{
  return m_instance;
}

ShrikeFrame* ShrikeFrame::m_instance = 0;
