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

class FragmentBranching : public Shader {
public:
  FragmentBranching();
  ~FragmentBranching();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static FragmentBranching instance;
};

FragmentBranching::FragmentBranching()
  : Shader("Branching: Fragment Unit")
{
}

FragmentBranching::~FragmentBranching()
{
}

bool FragmentBranching::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
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

FragmentBranching FragmentBranching::instance = FragmentBranching();


