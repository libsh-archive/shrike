// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
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
#include <iostream>
#include <fstream>
#include <wx/splitter.h>
#include <wx/colordlg.h>
#include <wx/treectrl.h>
#include <sh/ShObjMesh.hpp>
#include "ShrikeFrame.hpp"
#include "ShrikeCanvas.hpp"
#include "Shader.hpp"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace SH;
using namespace ShUtil;

BEGIN_EVENT_TABLE(ShrikeFrame, wxFrame)
  EVT_MENU(SHRIKE_MENU_OPEN_MODEL, ShrikeFrame::openModel)
  EVT_MENU(SHRIKE_MENU_QUIT, ShrikeFrame::quit)
  EVT_MENU(SHRIKE_MENU_SHADER_PROPS, ShrikeFrame::shaderProps)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSH, ShrikeFrame::showVsh)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSH, ShrikeFrame::showFsh)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSHIR, ShrikeFrame::showVshIr)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSHIR, ShrikeFrame::showFshIr)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSHIF, ShrikeFrame::showVshInterface)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSHIF, ShrikeFrame::showFshInterface)
  EVT_MENU(SHRIKE_MENU_SHADER_REINIT, ShrikeFrame::reinit)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTIMIZE, ShrikeFrame::optimize)

  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_LIFTING, ShrikeFrame::setopts)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_PROPAGATION, ShrikeFrame::setopts)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_DEADCODE, ShrikeFrame::setopts)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_SUBST, ShrikeFrame::setopts)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_COPY, ShrikeFrame::setopts)
  EVT_MENU(SHRIKE_MENU_SHADER_OPTS_STRAIGHT, ShrikeFrame::setopts)
  
  EVT_MENU(SHRIKE_MENU_VIEW_RESET, ShrikeFrame::resetView)
  EVT_MENU(SHRIKE_MENU_VIEW_SCREENSHOT, ShrikeFrame::screenshot)
  EVT_MENU(SHRIKE_MENU_VIEW_BACKGROUND, ShrikeFrame::setBackground)
  EVT_MENU(SHRIKE_MENU_VIEW_FULLSCREEN, ShrikeFrame::fullscreen)
  EVT_MENU(SHRIKE_MENU_VIEW_WIREFRAME, ShrikeFrame::wireframe)
  EVT_MENU(SHRIKE_MENU_VIEW_FPS, ShrikeFrame::fps)
  EVT_CLOSE(ShrikeFrame::close)
  EVT_KEY_DOWN(ShrikeFrame::keyDown)
  //  EVT_LISTBOX(SHRIKE_LISTBOX_SHADERS, ShrikeFrame::onShaderSelect)
  EVT_TREE_SEL_CHANGED(SHRIKE_TREECTRL_SHADERS, ShrikeFrame::onShaderSelect)
END_EVENT_TABLE()

struct ShaderTreeData : public wxTreeItemData {
  ShaderTreeData(Shader* shader)
    : shader(shader)
  {
  }
  
  Shader* shader;
};
  
ShrikeFrame::ShrikeFrame()
  : wxFrame(0, -1, "Shrike", wxDefaultPosition, wxSize(600, 400)),
    m_shader(0), m_fullscreen(false), m_fps(false)
{
  m_instance = this;
  CreateStatusBar();

  GetStatusBar()->SetStatusText("Hold down shift to rotate the light instead of the camera.");
  
  // Setup menus

  wxMenu* fileMenu = new wxMenu();
  fileMenu->Append(SHRIKE_MENU_OPEN_MODEL, "&Open Model...");
  fileMenu->AppendSeparator();
  fileMenu->Append(SHRIKE_MENU_QUIT, "&Quit");

  m_shaderMenu = new wxMenu();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_PROPS, "&Properties");
  m_shaderMenu->AppendSeparator();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_REINIT, "Re&initialize");
  m_shaderMenu->AppendSeparator();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSHIF, "Show &vertex interface");
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSHIF, "Show &fragment interface");
  m_shaderMenu->AppendSeparator();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSH, "Show &vertex assembly");
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSH, "Show &fragment assembly");
  m_shaderMenu->AppendSeparator();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSHIR, "Show &vertex IR");
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSHIR, "Show &fragment IR");
  m_shaderMenu->AppendSeparator();
  wxMenuItem* optitem = new wxMenuItem(m_shaderMenu, SHRIKE_MENU_SHADER_OPTIMIZE,
                                       "Turn on &optimizations", "",
                                       wxITEM_CHECK);
  m_shaderMenu->Append(optitem);
  optitem->Check(true);

  wxMenu* optmenu = new wxMenu();
  m_shaderMenu->Append(SHRIKE_MENU_SHADER_OPTS, "Optimizations", optmenu);

  char* optnames[] = {"Uniform Lifting",
                      "Constant/Uniform Propagation",
                      "Dead Code Removal",
                      "Forward Substitution",
                      "Copy Propagation",
                      "Straightening",
                      0};
  ShrikeId optids[] = {SHRIKE_MENU_SHADER_OPTS_LIFTING,
                       SHRIKE_MENU_SHADER_OPTS_PROPAGATION,
                       SHRIKE_MENU_SHADER_OPTS_DEADCODE,
                       SHRIKE_MENU_SHADER_OPTS_SUBST,
                       SHRIKE_MENU_SHADER_OPTS_COPY,
                       SHRIKE_MENU_SHADER_OPTS_STRAIGHT};
  for (int i = 0; optnames[i]; i++) {
    wxMenuItem* item = new wxMenuItem(optmenu,
                                      optids[i],
                                      optnames[i],
                                      "",
                                      wxITEM_CHECK);
    optmenu->Append(item);
    item->Check(true);
  }

  m_viewMenu = new wxMenu();
  m_viewMenu->Append(SHRIKE_MENU_VIEW_RESET, "&Reset");
  m_viewMenu->Append(SHRIKE_MENU_VIEW_BACKGROUND, "Set &background colour...");
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FULLSCREEN, "&Fullscreen");
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_WIREFRAME, "&Wireframe");
  m_viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FPS, "Show framera&te");
  m_viewMenu->Append(SHRIKE_MENU_VIEW_SCREENSHOT, "&Screenshot...");

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(m_shaderMenu, "&Shader");
  menuBar->Append(m_viewMenu, "&View");
  
  SetMenuBar(menuBar);

  m_hsplitter = new wxSplitterWindow(this, -1);

  m_shaderList = initShaderList(m_hsplitter);
  
  // Probably should do this somewhere else...
  ShObjMesh* model = 0;

  // TODO: FIXME
  std::ifstream infile(SHMEDIA_DIR "/objs/plane5.obj");
  if (infile) {
    model = new ShObjMesh(infile);
  }

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
  m_right_window = new wxSplitterWindow(m_hsplitter, -1);
  m_canvas = new ShrikeCanvas(m_right_window, model);
  m_panel = new UniformPanel(m_right_window);
  m_hsplitter->SplitVertically(m_shaderList, m_right_window, 150);
  m_right_window->SplitHorizontally(m_canvas, m_panel, -100);
  m_right_window->SetMinimumPaneSize(40);
#endif

}

ShrikeFrame::~ShrikeFrame()
{
}

void ShrikeFrame::openModel(wxCommandEvent& event)
{
  wxFileDialog* dialog = new wxFileDialog(this, "Open Model",
                                          SHMEDIA_DIR "/objs", "",
                                          "OBJ Files (*.obj)|*.obj", wxOPEN);
  if (dialog->ShowModal() == wxID_OK) {
    std::ifstream infile(dialog->GetPath());
    if (infile) {
      ShObjMesh* model = new ShObjMesh(infile);
      m_canvas->setModel(model);
    } // TODO: Complain if opening file failed
  }
}

void ShrikeFrame::shaderProps(wxCommandEvent& event)
{
  if (!m_shader) return;
  if (m_shader->paramCount() == 0) return;
  ShrikePropsDialog dialog(this, m_shader);
  
  if (dialog.ShowModal() != wxID_OK) return;

  try {
    m_shader->init();
  } catch (const ShException& e) {
    std::cerr << e.message() << std::endl;
    return;
  } catch (...) {
    std::cerr << "Unknown exception caught!" << std::endl;
    return;
  }
  setShader(m_shader);
}

void ShrikeFrame::quit(wxCommandEvent& event)
{
  Close(true);
}

void ShrikeFrame::close(wxCloseEvent& event)
{
  Destroy();
}

void ShrikeFrame::onShaderSelect(wxTreeEvent& event)
{
  wxTreeItemId item = event.GetItem();
  ShaderTreeData* data =
    dynamic_cast<ShaderTreeData*>(m_shaderList->GetItemData(item));
  if (!data) return;
  Shader* shader = data->shader;
  if (!setShader(shader)) {
    m_shaderList->SetItemTextColour(item, *wxRED);
  }
}

bool ShrikeFrame::setShader(Shader* shader)
{
  if (shader->failed()) {
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
    showError("An Image error occured trying to initialize or bind this program.\n"
              "This probably indicates a missing or corrupt texture image.",
              e.message());
    return false;
  } catch (const ShBackendException& e) {
    shader->set_failed(true);
    showError("A Backend error occured trying to initialize or bind this program.\n"
              "This probably indicates that your graphics card\n"
              "does not have the resources required to run this program.",
              e.message());
    return false;
  } catch (const ShException& e) {
    shader->set_failed(true);
    showError("An Sh error occured trying to initialize or bind this program.\n"
              "This probably indicates an error in the shader program,\n"
              "or an error trying to run the shader on your hardware\n"
              "(e.g. running out of instructions). It may also be a missing\n"
              "or corrupt texture image.",
              e.message());
    return false;
  } catch (...) {
    shader->set_failed(true);
    showError("An Unknown error occured trying to initialize or bind this program.\n"
              "This probably indicates an error in the shader program.");
    return false;
  }
  m_canvas->setShader(shader);
  m_canvas->render();
  m_panel->setShader(shader);
  m_shader = shader;
  return true;
}

void ShrikeFrame::showError(const std::string& message,
                            const std::string& details)
{
  if (!details.empty()) {
    wxLogWarning(details.c_str());
  }
  wxLogError(message.c_str());
  wxLog::FlushActive();
}

void ShrikeFrame::showVsh(wxCommandEvent& event)
{
  if (!m_shader) return;
  showProgram(m_shader->vertex(), "Vertex");
}

void ShrikeFrame::showFsh(wxCommandEvent& event)
{
  if (!m_shader) return;
  showProgram(m_shader->fragment(), "Fragment");
}

void ShrikeFrame::showVshIr(wxCommandEvent& event)
{
  if (!m_shader) return;
  showIR(m_shader->vertex(), "Vertex");
}

void ShrikeFrame::showFshIr(wxCommandEvent& event)
{
  if (!m_shader) return;
  showIR(m_shader->fragment(), "Fragment");
}

void ShrikeFrame::showVshInterface(wxCommandEvent& event)
{
  if (!m_shader) return;
  showInterface(m_shader->vertex(), "Vertex");
}

void ShrikeFrame::showFshInterface(wxCommandEvent& event)
{
  if (!m_shader) return;
  showInterface(m_shader->fragment(), "Fragment");
}

void ShrikeFrame::reinit(wxCommandEvent& event)
{
  if (!m_shader) return;
  try {
    m_shader->init();
  } catch (const ShException& e) {
    std::cerr << e.message() << std::endl;
    return;
  }
  setShader(m_shader);
}

void ShrikeFrame::resetView(wxCommandEvent& event)
{
  m_canvas->resetView();
}

void ShrikeFrame::setBackground(wxCommandEvent& event)
{
  wxColourDialog dialog(this);

  if (dialog.ShowModal() != wxID_OK) return;

  wxColour c = dialog.GetColourData().GetColour();
  m_canvas->setBackground(c.Red(), c.Green(), c.Blue());
}

void ShrikeFrame::showProgram(ShProgram program,
                              const std::string& name)
{
  if (!program.node()) return;
  if (!program.node()->code()) return;

  std::string title = name + " Shader Code";
  wxFrame* frame = new wxFrame(0, -1, title.c_str());


  wxTextCtrl* control = new wxTextCtrl(frame, -1, "",
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);

#ifndef NO_TEXT_WINDOW_STREAM
  std::ostream stream(control);
  program.node()->code()->print(stream);
#else
  std::ostringstream s;
  program.node()->code()->print(s);
  control->AppendText(s.str().c_str());
#endif

  frame->Show();
}

void ShrikeFrame::showInterface(ShProgram program,
                                const std::string& name)
{
  if (!program.node()) return;
  if (!program.node()->code()) return;

  std::string title = name + " Shader Interface";
  wxFrame* frame = new wxFrame(0, -1, title.c_str());

  wxTextCtrl* control = new wxTextCtrl(frame, -1, "",
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);
  
  control->AppendText(program.describe_interface().c_str());

  frame->Show();
}

void ShrikeFrame::showIR(ShProgram program,
                         const std::string& name)
{
  if (!program.node()) return;

  std::string title = name + " Shader Code";
  wxFrame* frame = new wxFrame(0, -1, title.c_str());


  wxTextCtrl* control = new wxTextCtrl(frame, -1, "",
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);

#ifndef NO_TEXT_WINDOW_STREAM
  std::ostream stream(control);
  program.node()->ctrlGraph->print(stream, 0);
#else
  std::ostringstream s;
  program.node()->ctrlGraph->print(s, 0);
  control->AppendText(s.str().c_str());
#endif

  frame->Show();
}

void ShrikeFrame::fullscreen(wxCommandEvent& event)
{
  setFullscreen(event.IsChecked());
}

void ShrikeFrame::optimize(wxCommandEvent& event)
{
  if (event.IsChecked()) {
    ShContext::current()->optimization(2);
  } else {
    ShContext::current()->optimization(0);
  }
}

void ShrikeFrame::setopts(wxCommandEvent& event)
{
  std::string name;
  switch (event.GetId()) {
  case SHRIKE_MENU_SHADER_OPTS_LIFTING:
    name = "uniform lifting";
    break;
  case SHRIKE_MENU_SHADER_OPTS_PROPAGATION:
    name = "propagation";
    break;
  case SHRIKE_MENU_SHADER_OPTS_DEADCODE:
    name = "deadcode";
    break;
  case SHRIKE_MENU_SHADER_OPTS_SUBST:
    name = "forward substitution";
    break;
  case SHRIKE_MENU_SHADER_OPTS_COPY:
    name = "copy propagation";
    break;
  case SHRIKE_MENU_SHADER_OPTS_STRAIGHT:
    name = "straightening";
    break;
  default:
    return;
  }
  if (event.IsChecked()) {
    ShContext::current()->enable_optimization(name);
  } else {
    ShContext::current()->disable_optimization(name);
  }
}

void ShrikeFrame::wireframe(wxCommandEvent& event)
{
  m_canvas->SetCurrent();
  
  if (event.IsChecked()) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  m_canvas->Refresh(FALSE);
}

void ShrikeFrame::setFullscreen(bool fs)
{
  ShowFullScreen(fs);
  if (fs) {
    m_hsplitter->Unsplit(m_shaderList);
  } else {
    if (!m_hsplitter->IsSplit()) {
      m_hsplitter->SplitVertically(m_shaderList, m_right_window,
                                   150);
      m_shaderList->Show();
    }
  }
  m_viewMenu->Check(SHRIKE_MENU_VIEW_FULLSCREEN, fs);
  m_fullscreen = fs;
}

void ShrikeFrame::fps(wxCommandEvent& event)
{
  setFps(event.IsChecked());
}

void ShrikeFrame::setFps(bool fps)
{
  m_fps = fps;
  ShrikeCanvas::instance()->setShowFps(m_fps);
}

void ShrikeFrame::keyDown(wxKeyEvent& event)
{
  if (event.GetKeyCode() == WXK_ESCAPE) {
    if (m_fullscreen) {
      setFullscreen(false);
    }
  } else {
    event.Skip();
  }
}

ShrikeFrame* ShrikeFrame::instance()
{
  return m_instance;
}

void ShrikeFrame::screenshot(wxCommandEvent& event)
{
  wxFileDialog* dialog = new wxFileDialog(this, "Save Screenshot",
                                          ".", "",
                                          "PNG Files (*.png)|*.png", wxSAVE);
  if (dialog->ShowModal() == wxID_OK) {
    m_canvas->screenshot(dialog->GetPath().c_str());
  }
}

wxTreeCtrl* ShrikeFrame::initShaderList(wxWindow* parent)
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

  wxTreeItemId root = tree->AddRoot("");

  std::map<std::string, wxTreeItemId> nodes;
  
  for (Shader::iterator I = Shader::begin(); I != Shader::end(); ++I) {
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
        nodes[fullname] = tree->AppendItem(last, name.c_str(), -1, -1,
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

ShrikeFrame* ShrikeFrame::m_instance = 0;

