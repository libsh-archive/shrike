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
#include <shutil/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class FragmentLooping : public Shader {
public:
  FragmentLooping(const Globals&);
  ~FragmentLooping();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

FragmentLooping::FragmentLooping(const Globals& globals)
  : Shader("Branching: Loops in Fragment Unit", globals)
{
}

FragmentLooping::~FragmentLooping()
{
}

bool FragmentLooping::init()
{
  vsh = ShKernelLib::shVsh( m_globals.mv, m_globals.mvp );
  vsh = vsh << shExtract("lightPos") << m_globals.lightPos; 
  vsh = shSwizzle("texcoord", "posh") << vsh;

  
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(julia_max_iter) = 20.0;
  julia_max_iter.range(1.0, 40.0);
  ShAttrib2f SH_DECL(julia_c) = ShAttrib2f(0.54, -0.51);
  julia_c.range(-1.0, 1.0);
  ShAttrib1f SH_DECL(brightness) = 1.0;
  brightness.range(0.0, 10.0);
  ShAttrib1f SH_DECL(height) = 0.2;
  height.range(0.0, 4.0);

  ShAttrib1f SH_DECL(zoom) = 0.33;
  zoom.range(0.0, 10.0);

  ShPoint2f SH_DECL(centre) = ShPoint2f(0.0, 0.0);
  centre.range(-2.0, 2.0);
  
  ShAttrib1f SH_DECL(gamma) = 0.7;
  gamma.range(0.0, 1.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f u;
    
    ShOutputColor3f ocol; // Color of result

    u = u - centre;
    u /= zoom;
    
    ShAttrib1f i = 0.0; 
    SH_DO {
      ShTexCoord2f v;
      v(0) = u(0)*u(0) - u(1)*u(1);
      v(1) = 2.0 * u(0) * u(1);
      u = v + julia_c;
      i += 1.0;
    } SH_UNTIL (0 == ((dot(u, u) < 2.0) * (i < julia_max_iter)));

    ShAttrib1f disp = pow(i / julia_max_iter, gamma);
    ocol = disp * diffuse * brightness;
  } SH_END;
  
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new FragmentLooping(globals));
    return list;
  }
}
#else
static StaticLinkedShader<FragmentLooping> instance = 
       StaticLinkedShader<FragmentLooping>();
#endif
