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

