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

class DiffuseShader : public Shader {
public:
  DiffuseShader();
  ~DiffuseShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static DiffuseShader instance;
};

DiffuseShader::DiffuseShader()
  : Shader("Basic Lighting Models: Diffuse: Algebra")
{
}

DiffuseShader::~DiffuseShader()
{
}

bool DiffuseShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShColor3f SH_DECL(color) = ShColor3f(.5, 0.9, 0.2);
  ShConstColor3f SH_DECL(lightColor) = ShColor3f(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::diffuse<ShColor3f>() << color << lightColor;
  return true;
}

DiffuseShader DiffuseShader::instance = DiffuseShader();


