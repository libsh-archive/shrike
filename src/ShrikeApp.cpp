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
#include "ShrikeApp.hpp"
#include "ShrikeFrame.hpp"
#include "Globals.hpp"
#include <sh/sh.hpp>
#include <wx/dir.h>
#include <wx/dynlib.h>
#include <wx/filename.h>

struct LibraryTraverser : public wxDirTraverser
{
  virtual wxDirTraverseResult OnFile(const wxString &file) {
    wxFileName fileName(file);

    if (wxT(".")+fileName.GetExt() != wxDynamicLibrary::GetDllExt())
      return wxDIR_CONTINUE;

    wxDynamicLibrary *dl = new wxDynamicLibrary(file);
    if (dl->IsLoaded()) {
      typedef ShaderList (*shrike_library_create_func)(const Globals&);
      shrike_library_create_func f = (shrike_library_create_func)dl->GetSymbol(wxT("shrike_library_create"));
      if (f != NULL) { 
        ShaderList list = (*f)(GetGlobals());
        for (ShaderList::iterator I = list.begin(); I != list.end(); ++I)
          GetShaders().push_back(*I);
    	return wxDIR_CONTINUE;
      }
    }
    delete dl;
    return wxDIR_CONTINUE;
  }
  virtual wxDirTraverseResult OnDir(const wxString &dir) {
    return wxDIR_CONTINUE;
  }
};

IMPLEMENT_APP(ShrikeApp)

ShrikeApp::ShrikeApp()
{
}
  
bool ShrikeApp::OnInit()
{
  std::string backend_name = "arb";

  if (argc > 1) {
    backend_name = wxConvLibc.cWX2MB(argv[1]);
  }
  
  SH::shSetBackend(backend_name);

  GetGlobals().lightPos = SH::ShPoint3f(0.0, 10.0, 10.0);
  GetGlobals().lightDirW = SH::ShVector3f(0.0, 1.0, 1.0);
  GetGlobals().lightLenW = 5.0;
  GetGlobals().mv = SH::ShMatrix4x4f();
  GetGlobals().mv_inverse = SH::ShMatrix4x4f();
  GetGlobals().mvp = SH::ShMatrix4x4f();
  
  LibraryTraverser t;
  wxString envLibDir;
  if (wxGetEnv(wxT("SHRIKE_LIB_DIR"), &envLibDir) && envLibDir != wxT("")) {
    std::cout << "Loading shaders in " << envLibDir << std::endl;
    if (wxDir::Exists(envLibDir)) {
      wxDir dir(envLibDir);
      dir.Traverse(t);
    }
  }
  std::cout << "Loading shaders in " << SHRIKE_LIB_DIR << std::endl;
  if (wxDir::Exists(wxT(SHRIKE_LIB_DIR))) {
    wxDir libDir(wxT(SHRIKE_LIB_DIR));
    libDir.Traverse(t);
  }

  ShrikeFrame* frame = new ShrikeFrame();
  frame->Show(true);
  
  return true;
}

