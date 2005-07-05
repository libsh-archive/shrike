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
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class PhongShader : public Shader {
public:
  PhongShader();
  ~PhongShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

PhongShader::PhongShader()
  : Shader("Basic Lighting Models: Phong: Algebra")
{
}

PhongShader::~PhongShader()
{
}

bool PhongShader::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal","halfVec", "lightVec", "posh", "texcoord") << vsh;

  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShImage image;
  image.loadPng(normalize_path(SHMEDIA_DIR "/textures/rustkd.png"));
  ShTable2D<ShColor3fub> difftex(image.width(), image.height());
  difftex.memory(image.memory());
  image.loadPng(normalize_path(SHMEDIA_DIR "/textures/rustks.png"));
  ShTable2D<ShColor3fub> spectex(image.width(), image.height());
  spectex.memory(image.memory());
  
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::phong<ShColor3f>();
  fsh = fsh << namedCombine(access(difftex), access(spectex));
  fsh = fsh << shExtract("specExp") << exponent;
  fsh = fsh << shExtract("irrad") << lightColor;
  return true;
}

PhongShader the_phong_shader;

