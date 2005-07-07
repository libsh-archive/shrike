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
