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
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class FragmentBranching : public Shader {
public:
  FragmentBranching(const Globals&);
  ~FragmentBranching();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

FragmentBranching::FragmentBranching(const Globals& globals)
  : Shader("Branching: Fragment Unit", globals)
{
}

FragmentBranching::~FragmentBranching()
{
}

bool FragmentBranching::init()
{
  vsh = ShKernelLib::shVsh( m_globals.mv, m_globals.mvp );
  vsh = vsh << shExtract("lightPos") << m_globals.lightPos; 
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShColor3f SH_DECL(color1) = ShColor3f(1.0, 1.0, 0.0);
  ShColor3f SH_DECL(color2) = ShColor3f(0.0, 0.0, 1.0);
  ShAttrib1f SH_DECL(cutoff) = 0.5;

  ShAttrib2f SH_DECL(julia_c) = ShAttrib2f(0.33, -0.51);
  julia_c.range(-1.0, 1.0);

  int iterations = 80;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc;
    ShOutputColor3f out;

    SH_IF(tc(0) < cutoff) {
      ShAttrib2f u = tc;
      ShAttrib1f its = 0.0;
      for (int i = 0; i < iterations; i++) {
        ShTexCoord2f v;
        v(0) = u(0)*u(0) - u(1)*u(1);
        v(1) = 2.0 * u(0) * u(1);
        u = v + julia_c;
        
        its = cond(dot(u,u) < 2.0, its + 1.0, its);
      }
      out = its/(float)(iterations) * color1;
    } SH_ELSE {
      out = color2;
    } SH_ENDIF;
    
  } SH_END;

  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new FragmentBranching(globals));
    return list;
  }
}
#else
static StaticLinkedShader<FragmentBranching> instance = 
       StaticLinkedShader<FragmentBranching>();
#endif
