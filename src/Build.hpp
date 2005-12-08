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
#ifndef BUILD_HPP
#define BUILD_HPP

#include <wx/wx.h>
#include <wx/process.h>
#include "Project.hpp"

class BuildPanel : public wxPanel
{
public:
  BuildPanel(wxWindow* parent);

  void save_config();
private:
  void open_config();
  
  void on_compiler_change(wxCommandEvent& e);
  void on_include_add(wxCommandEvent& e);
  void on_include_rem(wxCommandEvent& e);
  void on_library_add(wxCommandEvent& e);
  void on_library_rem(wxCommandEvent& e);
  
  wxSizer* create_compiler();
  wxSizer* create_includes();
  wxSizer* create_libraries();
  wxSizer* create_flags();

  wxTextCtrl* m_compiler;
  wxListBox* m_includes;
  wxListBox* m_libraries;
  wxTextCtrl* m_flags;

  DECLARE_EVENT_TABLE();
};

class BuildProcess : public wxProcess
{
public:
  BuildProcess() : wxProcess()
  {
    Redirect();
  }

  void OnTerminate(int pid, int status)
  {
    m_pid = pid;
    m_status = status;
  }

  int pid() const { return m_pid; }
  int status() const { return m_status; }
  void status(int status) { m_status = status; }
private:
  int m_pid, m_status;
};

BuildProcess* build_project(const Project& project);

#endif
