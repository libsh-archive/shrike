#include "Project.hpp"
#include <wx/fileconf.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>

Project::Project()
  : m_saved(false), m_dll(0)
{
}

Project::Project(const wxString& file)
  : m_target(file), m_dll(0)
{
  load_shaders();
}

Project::~Project()
{
  unload_shaders();
}

void Project::load_shaders()
{
  unload_shaders();
  
  wxFileName path(workspace(), target()+wxDynamicLibrary::GetDllExt());

  m_dll = new wxDynamicLibrary(path.GetFullPath());
  if (m_dll->IsLoaded() && m_dll->HasSymbol(wxT("shrike_library_create"))) {
    typedef ShaderList (*create_func)(const Globals&);
    create_func f = (create_func)m_dll->GetSymbol(wxT("shrike_library_create"));
    if (f != NULL)
      m_shaders = (*f)(GetGlobals());    
  }
}

void Project::unload_shaders()
{
  if (m_dll) {
    for (ShaderList::iterator I = m_shaders.begin(); I != m_shaders.end(); ++I)
      delete *I;
    m_shaders.clear();
    delete m_dll;
  }
  m_dll = 0;
}

bool Project::save()
{
  wxFileName fname(workspace(), config());

  // if you don't pass a file to wxFileConfig it reads the default
  // (eg ~/.shrike)  So read existing or new empty project file.
  wxFile file;
  file.Create(fname.GetFullPath(), false);
  wxFileInputStream ifs(fname.GetFullPath());
  wxFileConfig conf(ifs);
  wxFileOutputStream ofs(fname.GetFullPath());

  conf.Write(wxT("Name"), name());
  conf.Write(wxT("Target"), target());
  wxString sources(wxT(""));
  for (FileList::const_iterator I = begin_sources(); I != end_sources(); ++I, sources+=wxT(";"))
    sources += *I;
  conf.Write(wxT("Source"), sources);

  if (conf.Save(ofs)) {
    m_saved = true;
    return true;
  }
  return false;
}

bool Project::open(const wxString& file)
{
  wxFileInputStream ifs(file);
  wxFileConfig conf(ifs);

  wxString value;
  if (!conf.Read(wxT("Name"), &value)) 
    return false;
  name(value);

  if (!conf.Read(wxT("Target"), &value)) {
    wxFileName fn(file);
    value = fn.GetName();
  }
  target(value);
  
  if (conf.Read(wxT("Source"), &value)) {
    wxStringTokenizer tok(value, wxT(";"));
    while (tok.HasMoreTokens()) {
      wxString src=tok.GetNextToken();
      m_sources.push_back(src);
    }
  }
  
  wxFileName fname(file);
  workspace(fname.GetPath()); 
  config(fname.GetFullName());
  
  load_shaders();
  
  m_saved = true;
  
  return true;
}
