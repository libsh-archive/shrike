#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <wx/dynlib.h>
#include <wx/filename.h>
#include "Shader.hpp"

class Project
{
public:
  Project();
  Project(const wxString& dll_file);
  ~Project();

  // short name of project. eg Foo Shader
  void name(const wxString& name) { m_name=name; }
  const wxString& name() const { return m_name; }
  // directory of project files. eg /home/user/Shrike/Projects/Foo
  void workspace(const wxString& workspace) { m_workspace=workspace; }
  const wxString& workspace() const { return m_workspace; }
  // project config file eg foo.proj
  void config(const wxString& config) { m_config=config; }
  const wxString& config() const { return m_config; }
  // build target without extension. eg foo -> foo.dll (Windows) or libfoo.so (Unix)
  void target(const wxString& target) { m_target=target; }
  const wxString& target() const { return m_target; }

  void load_shaders();
  void unload_shaders();

  // Shader iterators
  ShaderList::iterator begin_shaders() { return m_shaders.begin(); }
  ShaderList::iterator end_shaders() { return m_shaders.end(); }

  typedef std::list<wxString> FileList;
  FileList& sources() { m_saved=false; return m_sources; }
  FileList::const_iterator begin_sources() const { return m_sources.begin(); }
  FileList::const_iterator end_sources() const { return m_sources.end(); }

  bool saved() const { return m_saved; }
  bool save();
  bool open(const wxString& file);
private:
  bool m_saved;

  wxString m_name;
  wxString m_workspace;
  wxString m_config;
  wxString m_target;
  FileList m_sources;

  wxDynamicLibrary* m_dll;

  ShaderList m_shaders;
};

typedef std::list<Project*> ProjectList;

#endif
