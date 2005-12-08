#include "Build.hpp"
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/propdlg.h>
#include <wx/tokenzr.h>

enum ShrikeBuildPanelId{
  SHRIKE_BUILD_PANEL_COMPILER_CHANGE = wxID_HIGHEST+1,
  SHRIKE_BUILD_PANEL_INCLUDE_ADD,
  SHRIKE_BUILD_PANEL_INCLUDE_REM,
  SHRIKE_BUILD_PANEL_LIBRARY_ADD,
  SHRIKE_BUILD_PANEL_LIBRARY_REM
};

BuildPanel::BuildPanel(wxWindow* parent) : wxPanel(parent, -1)
{
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(create_compiler(),0, wxEXPAND);
  sizer->Add(create_includes(),1, wxEXPAND);
  sizer->Add(create_libraries(),1, wxEXPAND);
  sizer->Add(create_flags(),1, wxEXPAND);

  open_config();
    
  SetSizer(sizer);
  Show(true);
}

void BuildPanel::open_config()
{
  wxConfig config(wxT("shrike"));

  if (!config.HasGroup(wxT("Build"))) {
    wxMessageDialog dialog(this, 
      wxT("Shrike could not find your build settings. Default values will be filled in for you.\n"
          "\n"
          "Please manually add the location of Shrike.hpp to Include paths\n"
          "Please manually add the location of libshrike.la to Library paths\n"
          "\n"
          "NOTE: Shrike only supports GCC.  Support for other compilers is in progress"), 
      wxT("Missing build settings"), wxOK | wxICON_EXCLAMATION);
    dialog.ShowModal();

    m_compiler->SetValue(wxT("g++"));
    m_flags->SetValue(wxT("-shared -lsh -lshutil -lshrike"));
    return;
  }
  
  wxString value;
  if (config.Read(wxT("Build/Compiler"), &value)) {
    m_compiler->SetValue(value);
  }
  if (config.Read(wxT("Build/IncludePaths"), &value)) {
    wxStringTokenizer tok(value, wxT(";"));
    while (tok.HasMoreTokens())
      m_includes->Insert(tok.GetNextToken(),0);
  }
  if (config.Read(wxT("Build/LibraryPaths"), &value)) {
    wxStringTokenizer tok(value, wxT(";"));
    while (tok.HasMoreTokens())
      m_libraries->Insert(tok.GetNextToken(),0);
  }
  if (config.Read(wxT("Build/Flags"), &value)) {
    m_flags->SetValue(value);
  }
}
 
void BuildPanel::save_config()
{
  wxConfig config(wxT("shrike"));

  config.Write(wxT("Build/Compiler"), m_compiler->GetValue());

  wxString value(wxT(""));
  for (int i = 0; i < m_includes->GetCount(); ++i, value+=wxT(";"))
    value += m_includes->GetString(i);
  config.Write(wxT("Build/IncludePaths"), value);

  value = wxT("");
  for (int i = 0; i < m_libraries->GetCount(); ++i, value+=wxT(";"))
    value += m_libraries->GetString(i);
  config.Write(wxT("Build/LibraryPaths"), value);

  config.Write(wxT("Build/Flags"), m_flags->GetValue());
}

void BuildPanel::on_compiler_change(wxCommandEvent& e)
{
  wxFileDialog dialog(this, wxT("Choose compiler"));
  if (dialog.ShowModal() != wxID_OK) return;
  m_compiler->SetValue(dialog.GetPath());
}

void BuildPanel::on_include_add(wxCommandEvent& e)
{
  wxDirDialog dialog(this, wxT("Choose include path"));
  if (dialog.ShowModal() != wxID_OK) return;
  m_includes->Insert(dialog.GetPath(),0);
}

void BuildPanel::on_include_rem(wxCommandEvent& e)
{
  int i = m_includes->GetSelection();
  if (i == wxNOT_FOUND) return;
  m_includes->Delete(i);
}

void BuildPanel::on_library_add(wxCommandEvent& e)
{
  wxDirDialog dialog(this, wxT("Choose library path"));
  if (dialog.ShowModal() != wxID_OK) return;
  m_libraries->Insert(dialog.GetPath(),0);
}

void BuildPanel::on_library_rem(wxCommandEvent& e)
{
  int i = m_libraries->GetSelection();
  if (i == wxNOT_FOUND) return;
  m_libraries->Delete(i);
}
  
wxSizer* BuildPanel::create_compiler()
{
  wxStaticBox* sb = new wxStaticBox(this, -1, wxT("Compiler"));
  wxStaticBoxSizer* hbox = new wxStaticBoxSizer(sb, wxHORIZONTAL);
  m_compiler = new wxTextCtrl(this, -1);
  hbox->Add(m_compiler, 1, wxEXPAND);
  hbox->Add(new wxButton(this, SHRIKE_BUILD_PANEL_COMPILER_CHANGE, wxT("Change")));
  return hbox;
}

wxSizer* BuildPanel::create_includes()
{
  wxStaticBox* sb = new wxStaticBox(this, -1, wxT("Include paths"));
  wxStaticBoxSizer* sbs = new wxStaticBoxSizer(sb, wxHORIZONTAL);
  m_includes = new wxListBox(this, -1);
  wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
  vbox->Add(new wxButton(this, SHRIKE_BUILD_PANEL_INCLUDE_ADD, wxT("Add")));
  vbox->Add(new wxButton(this, SHRIKE_BUILD_PANEL_INCLUDE_REM, wxT("Remove")));
  sbs->Add(m_includes, 1, wxEXPAND);
  sbs->Add(vbox);
  return sbs;
}

wxSizer* BuildPanel::create_libraries()
{
  wxStaticBox* sb = new wxStaticBox(this, -1, wxT("Library paths"));
  wxStaticBoxSizer* sbs = new wxStaticBoxSizer(sb, wxHORIZONTAL);
  m_libraries = new wxListBox(this, -1);
  wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
  vbox->Add(new wxButton(this, SHRIKE_BUILD_PANEL_LIBRARY_ADD, wxT("Add")));
  vbox->Add(new wxButton(this, SHRIKE_BUILD_PANEL_LIBRARY_REM, wxT("Remove")));
  sbs->Add(m_libraries, 1, wxEXPAND);
  sbs->Add(vbox);
  return sbs;
}

wxSizer* BuildPanel::create_flags()
{
  wxStaticBox* sb = new wxStaticBox(this, -1, wxT("Additional flags"));
  wxStaticBoxSizer* sbs = new wxStaticBoxSizer(sb, wxHORIZONTAL);
  m_flags = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
  sbs->Add(m_flags, 1, wxEXPAND);
  return sbs;
}

BEGIN_EVENT_TABLE(BuildPanel, wxPanel)
  EVT_BUTTON(SHRIKE_BUILD_PANEL_COMPILER_CHANGE, BuildPanel::on_compiler_change)
  EVT_BUTTON(SHRIKE_BUILD_PANEL_INCLUDE_ADD, BuildPanel::on_include_add)
  EVT_BUTTON(SHRIKE_BUILD_PANEL_INCLUDE_REM, BuildPanel::on_include_rem)
  EVT_BUTTON(SHRIKE_BUILD_PANEL_LIBRARY_ADD, BuildPanel::on_library_add)
  EVT_BUTTON(SHRIKE_BUILD_PANEL_LIBRARY_REM, BuildPanel::on_library_rem)
END_EVENT_TABLE()

BuildProcess* build_project(const Project& project)
{
  wxConfig config(wxT("shrike"));

  wxString value;
  if (!config.Read(wxT("Build/Compiler"), &value)) 
    return false;
  wxString cmd = value + wxT(" -o ") + project.target()+wxDynamicLibrary::GetDllExt();
  // add sources
  for (Project::FileList::const_iterator I = project.begin_sources(); I != project.end_sources(); ++I) {
    wxFileName fn(*I);
    if (fn.GetExt() == wxT("cpp"))
      cmd += wxT(" ")+*I;
  }
  // add include paths
  if (config.Read(wxT("Build/IncludePaths"), &value)) {
    wxStringTokenizer tok(value, wxT(";"));
    while (tok.HasMoreTokens())
      cmd += wxT(" -I")+tok.GetNextToken();
  }
  // add library paths 
  if (config.Read(wxT("Build/LibraryPaths"), &value)) {
    wxStringTokenizer tok(value, wxT(";"));
    while (tok.HasMoreTokens())
      cmd += wxT(" -L")+tok.GetNextToken();
  }
  if (config.Read(wxT("Build/Flags"), &value))
    cmd += wxT(" ") + value;

  wxString cwd = wxFileName::GetCwd();
  wxFileName::SetCwd(project.workspace()); 
  BuildProcess* process = new BuildProcess();
  process->status(wxExecute(cmd, wxEXEC_SYNC, process));
  wxFileName::SetCwd(cwd);
  return process;
}
