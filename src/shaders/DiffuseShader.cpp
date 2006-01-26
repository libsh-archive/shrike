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

class DiffuseShader : public Shader {
public:
  DiffuseShader(const Globals &globals);
  ~DiffuseShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

DiffuseShader::DiffuseShader(const Globals &globals)
  : Shader("Basic Lighting Models: Diffuse: Algebra", globals)
{
}

DiffuseShader::~DiffuseShader()
{
}

bool DiffuseShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( m_globals.mv, m_globals.mvp );
  vsh = vsh << shExtract("lightPos") << m_globals.lightPos; 
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShColor3f SH_DECL(color) = ShColor3f(.5, 0.9, 0.2);
  ShConstColor3f SH_DECL(lightColor) = ShColor3f(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::diffuse<ShColor3f>() << color << lightColor;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new DiffuseShader(globals));
    return list;
  }
}
#else
static StaticLinkedShader<DiffuseShader> instance = 
       StaticLinkedShader<DiffuseShader>();
#endif
