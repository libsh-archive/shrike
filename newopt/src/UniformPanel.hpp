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
#ifndef UNIFORMPANEL_HPP
#define UNIFORMPANEL_HPP

#include <list>
#include <wx/wx.h>
#include <wx/sizer.h>
#include "Shader.hpp"

class UniformPanel : public wxScrolledWindow {
public:
  UniformPanel(wxWindow* parent);

  void setShader(Shader* shader);

private:

  void addVar(const SH::ShVariableNodePtr& node,
              wxFlexGridSizer* sizer);
  
  wxBoxSizer* m_sizer;

  std::list<SH::ShVariableNodePtr> m_vars;
};

#endif