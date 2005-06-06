// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
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
#include "ShrikeApp.hpp"
#include "ShrikeFrame.hpp"
#include <sh/sh.hpp>

IMPLEMENT_APP(ShrikeApp)

ShrikeApp::ShrikeApp()
{
}
  
bool ShrikeApp::OnInit()
{
  std::string backend_name = "arb";

  if (argc > 1) {
    backend_name = argv[1];
  }
  
  SH::shSetBackend(backend_name);

  ShrikeFrame* frame = new ShrikeFrame;
  frame->Show(true);
  
  return true;
}

