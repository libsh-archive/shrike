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

class WobbleShader : public Shader {
public:
  WobbleShader();
  ~WobbleShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

WobbleShader::WobbleShader()
  : Shader("Animation: Wobble")
{
}

WobbleShader::~WobbleShader()
{
}

bool WobbleShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("normal") << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShAttrib1f SH_DECL(time);
  time.range(0.0f, 2.0 * M_PI * 10.0f);
  ShAttrib1f SH_DECL(scale) = 1.0f;
  scale.range(0.0f, 10.0f);
  ShAttrib1f SH_DECL(frequency) = 0.5f;
  frequency.range(0.0f, 5.0f);
  ShProgram wobbler = SH_BEGIN_PROGRAM() {
    ShInOutPosition4f pos;
    ShInOutNormal3f normal;

    ShAttrib1f disp = scale * 0.5 * (sin(pos(1) * frequency + time) + 1.0f);

    pos(0,1,2) += normal * disp;
  } SH_END;
  
  vsh = vsh << shExtract("posm") << wobbler;
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 1.0, 1.0);
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::diffuse<ShColor3f>() << diffuse << lightColor;
  return true;
}

WobbleShader the_wobble_shader;
