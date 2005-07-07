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

class Tangents : public Shader {
public:
  Tangents();
  ~Tangents();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static Tangents instance;
};

Tangents::Tangents()
  : Shader("Debugging: Tangents")
{
}

Tangents::~Tangents()
{
}

bool Tangents::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp);
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  vsh = shSwizzle("posh") << vsh;
  ShProgram keeper = SH_BEGIN_PROGRAM() {
    ShInOutVector3f tan; // Pass through untransformed tangents
  } SH_END_PROGRAM;
 vsh = vsh & keeper;
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputVector3f i;
    ShOutputColor3f o = i * 0.5f + 0.5f;
  } SH_END_PROGRAM;
  return true;
}


Tangents Tangents::instance = Tangents();

