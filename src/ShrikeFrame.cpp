#include <fstream>
#include <wx/splitter.h>
#include "ShrikeFrame.hpp"
#include "ShrikeCanvas.hpp"
#include "Shader.hpp"
#include "ShObjFile.hpp"

using namespace SH;

BEGIN_EVENT_TABLE(ShrikeFrame, wxFrame)
  EVT_MENU(SHRIKE_MENU_OPEN_MODEL, ShrikeFrame::openModel)
  EVT_MENU(SHRIKE_MENU_QUIT, ShrikeFrame::quit)
  EVT_LISTBOX(SHRIKE_LISTBOX_SHADERS, ShrikeFrame::setShader)
END_EVENT_TABLE()

ShrikeFrame::ShrikeFrame()
  : wxFrame(0, -1, "Shrike")
{
  CreateStatusBar();

  // Setup menus

  wxMenu* fileMenu = new wxMenu();
  fileMenu->Append(SHRIKE_MENU_OPEN_MODEL, "&Open Model");
  fileMenu->AppendSeparator();
  fileMenu->Append(SHRIKE_MENU_QUIT, "&Quit");

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, "&File");
  
  SetMenuBar(menuBar);

  wxSplitterWindow* hsplitter = new wxSplitterWindow(this, -1);

  wxListBox* shaderList = initShaderList(hsplitter);

  // Probably should do this somewhere else...
  ShObjFile* model = new ShObjFile();

  // TODO: FIXME
  std::ifstream infile("../../shmedia/objs/sphere50.obj");
  infile >> *model;

  wxSplitterWindow* right_window = new wxSplitterWindow(hsplitter, -1);
  m_canvas = new ShrikeCanvas(right_window, model);
  m_panel = new UniformPanel(right_window);
  
  hsplitter->SplitVertically(shaderList, right_window, 150);
  right_window->SplitHorizontally(m_canvas, m_panel, 600);
}

ShrikeFrame::~ShrikeFrame()
{
}

void ShrikeFrame::openModel(wxCommandEvent& event)
{
  wxFileDialog* dialog = new wxFileDialog(this, "Open Model",
                                          "../../shmedia/objs", "",
                                          "OBJ Files (*.obj)|*.obj", wxOPEN);
  if (dialog->ShowModal() == wxID_OK) {
    std::ifstream infile(dialog->GetPath());
    if (infile) {
      ShObjFile* model = new ShObjFile();
      infile >> *model;
      m_canvas->setModel(model);
    } // TODO: Complain if opening file failed
  }
}

void ShrikeFrame::quit(wxCommandEvent& event)
{
  Close(true);
}

void ShrikeFrame::setShader(wxCommandEvent& event)
{
  Shader* shader = reinterpret_cast<Shader*>(event.GetClientData());
  m_canvas->SetCurrent();
  shader->bind();
  m_canvas->render();
  m_panel->setShader(shader);
}

wxListBox* ShrikeFrame::initShaderList(wxWindow* parent)
{
  wxListBox* box = new wxListBox(parent, SHRIKE_LISTBOX_SHADERS,
                                 wxDefaultPosition, wxDefaultSize, 0, 0,
                                 wxLB_SINGLE);

  for (Shader::iterator I = Shader::begin(); I != Shader::end(); ++I) {
    Shader* shader = *I;
    if (shader->init()) {
      box->Append(shader->name().c_str(), shader);
    }
  }
  return box;
}

