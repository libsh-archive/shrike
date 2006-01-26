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

class PhongShader : public Shader {
public:
  PhongShader(const Globals &);
  ~PhongShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

PhongShader::PhongShader(const Globals &globals)
  : Shader("Basic Lighting Models: Phong: Algebra", globals)
{
}

PhongShader::~PhongShader()
{
}

bool PhongShader::init()
{
  vsh = ShKernelLib::shVsh( m_globals.mv, m_globals.mvp );
  vsh = vsh << shExtract("lightPos") << m_globals.lightPos; 
  vsh = shSwizzle("normal","halfVec", "lightVec", "posh", "texcoord") << vsh;

  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShImage image;
  load_PNG(image, normalize_path(SHMEDIA_DIR "/textures/rustkd.png"));
  ShTable2D<ShColor3fub> difftex(image.width(), image.height());
  difftex.memory(image.memory());
  load_PNG(image, normalize_path(SHMEDIA_DIR "/textures/rustks.png"));
  ShTable2D<ShColor3fub> spectex(image.width(), image.height());
  spectex.memory(image.memory());
  
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::phong<ShColor3f>();
  fsh = fsh << namedCombine(shAccess(difftex), shAccess(spectex));
  fsh = fsh << shExtract("specExp") << exponent;
  fsh = fsh << shExtract("irrad") << lightColor;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new PhongShader(globals));
    return list;
  }
}
#else
static StaticLinkedShader<PhongShader> instance = 
       StaticLinkedShader<PhongShader>();
#endif
