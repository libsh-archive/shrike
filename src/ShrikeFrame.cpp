#include <iostream>
#include <fstream>
#include <wx/splitter.h>
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
  EVT_MENU(SHRIKE_MENU_SHADER_REINIT, ShrikeFrame::reinit)
  EVT_MENU(SHRIKE_MENU_VIEW_RESET, ShrikeFrame::resetView)
  EVT_LISTBOX(SHRIKE_LISTBOX_SHADERS, ShrikeFrame::onShaderSelect)
END_EVENT_TABLE()

ShrikeFrame::ShrikeFrame()
  : wxFrame(0, -1, "Shrike", wxDefaultPosition, wxSize(400, 600)),
    m_shader(0)
{
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
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_VSH, "Show &vertex assembly");
  shaderMenu->Append(SHRIKE_MENU_SHADER_SHOW_FSH, "Show &fragment assembly");

  wxMenu* viewMenu = new wxMenu();
  viewMenu->Append(SHRIKE_MENU_VIEW_RESET, "&Reset");
  
  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(shaderMenu, "&Shader");
  menuBar->Append(viewMenu, "&View");
  
  SetMenuBar(menuBar);

  wxSplitterWindow* hsplitter = new wxSplitterWindow(this, -1);

  wxListBox* shaderList = initShaderList(hsplitter);
  
  // Probably should do this somewhere else...
  ShObjMesh* model = 0;

  // TODO: FIXME
  std::ifstream infile(SHMEDIA_DIR "/objs/sphere50.obj");
  if (infile) {
    model = new ShObjMesh(infile);
  }

  wxSplitterWindow* right_window = new wxSplitterWindow(hsplitter, -1);
  m_canvas = new ShrikeCanvas(right_window, model);
  m_panel = new UniformPanel(right_window);
  
  hsplitter->SplitVertically(shaderList, right_window, 150);
  right_window->SplitHorizontally(m_canvas, m_panel, -100);
  right_window->SetMinimumPaneSize(40);
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

  m_shader->init();
  setShader(m_shader);
}

void ShrikeFrame::quit(wxCommandEvent& event)
{
  Close(true);
}

void ShrikeFrame::onShaderSelect(wxCommandEvent& event)
{
  Shader* shader = reinterpret_cast<Shader*>(event.GetClientData());
  setShader(shader);
}

void ShrikeFrame::setShader(Shader* shader)
{
  m_canvas->SetCurrent();
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

void ShrikeFrame::reinit(wxCommandEvent& event)
{
  if (!m_shader) return;
  m_shader->init();
  setShader(m_shader);
}

void ShrikeFrame::resetView(wxCommandEvent& event)
{
  m_canvas->resetView();
}

void ShrikeFrame::showProgram(const ShProgram& program,
                              std::string name)
{
  if (!program) return;
  if (!program->code(ShEnvironment::backend)) return;

  std::string title = name + " Shader Code";
  wxFrame* frame = new wxFrame(0, -1, title.c_str());

  wxTextCtrl* control = new wxTextCtrl(frame, -1, "",
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY);
  
  std::ostream stream(control);
  program->code(ShEnvironment::backend)->print(stream);

  frame->Show();
}


wxListBox* ShrikeFrame::initShaderList(wxWindow* parent)
{
  wxListBox* box = new wxListBox(parent, SHRIKE_LISTBOX_SHADERS,
                                 wxDefaultPosition, wxDefaultSize, 0, 0,
                                 wxLB_SINGLE | wxLB_SORT);

  for (Shader::iterator I = Shader::begin(); I != Shader::end(); ++I) {
    Shader* shader = *I;
    if (shader->init()) {
      box->Append(shader->name().c_str(), shader);
    }
  }
  return box;
}

