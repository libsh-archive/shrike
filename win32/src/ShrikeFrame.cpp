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
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_VSHIF, ShrikeFrame::showVshInterface)
  EVT_MENU(SHRIKE_MENU_SHADER_SHOW_FSHIF, ShrikeFrame::showFshInterface)
  EVT_MENU(SHRIKE_MENU_SHADER_REINIT, ShrikeFrame::reinit)
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
  fileMenu->Append(SHRIKE_MENU_OPEN_MODEL, "&Open Model");
  fileMenu->AppendSeparator();
  fileMenu->Append(SHRIKE_MENU_QUIT, "&Quit");

  wxMenu* shaderMenu = new wxMenu();
  shaderMenu->Append(SHRIKE_MENU_SHADER_PROPS, "&Properties");
  shaderMenu->AppendSeparator();
  shaderMenu->Append(SHRIKE_MENU_SHADER_REINIT, "Re&initialize");
  shaderMenu->AppendSeparator();
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSHIF, "Show &vertex interface");
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSHIF, "Show &fragment interface");
  shaderMenu->AppendSeparator();
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSH, "Show &vertex assembly");
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSH, "Show &fragment assembly");

  wxMenu* viewMenu = new wxMenu();
  viewMenu->Append(SHRIKE_MENU_VIEW_RESET, "&Reset");
  viewMenu->Append(SHRIKE_MENU_VIEW_BACKGROUND, "Set &background colour...");
  viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FULLSCREEN, "Fullscreen");
  viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_WIREFRAME, "&Wireframe");
  viewMenu->AppendCheckItem(SHRIKE_MENU_VIEW_FPS, "Show framerate");
  viewMenu->Append(SHRIKE_MENU_VIEW_SCREENSHOT, "Screenshot");

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(shaderMenu, "&Shader");
  menuBar->Append(viewMenu, "&View");
  
  SetMenuBar(menuBar);

  m_hsplitter = new wxSplitterWindow(this, -1);

  m_shaderList = initShaderList(m_hsplitter);
  
  // Probably should do this somewhere else...
  ShObjMesh* model = 0;

  // TODO: FIXME
  std::ifstream infile(SHMEDIA_DIR "/objs/sphere50.obj");
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
  }
  setShader(m_shader);
}

void ShrikeFrame::quit(wxCommandEvent& event)
{
  Destroy();
  //  Close(true);
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
  setShader(shader);
}

void ShrikeFrame::setShader(Shader* shader)
{
  m_canvas->SetCurrent();
  try {
    if (shader) shader->firstTimeInit();
  } catch (const ShException& e) {
    std::cerr << e.message() << std::endl;
    return;
  }
  if (shader) shader->bind();
  m_canvas->setShader(shader);
  m_canvas->render();
  m_panel->setShader(shader);
  m_shader = shader;
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

void ShrikeFrame::fullscreen(wxCommandEvent& event)
{
  setFullscreen(event.IsChecked());
}

void ShrikeFrame::wireframe(wxCommandEvent& event)
{
  m_canvas->SetCurrent();
  
  if (event.IsChecked()) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
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
  wxTreeCtrl* tree = new wxTreeCtrl(parent, SHRIKE_TREECTRL_SHADERS,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_NO_LINES);

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

