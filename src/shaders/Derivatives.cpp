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

class Derivatives : public Shader {
public:
  Derivatives();
  ~Derivatives();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static Derivatives instance;
};

Derivatives::Derivatives()
  : Shader("Debugging: Derivatives")
{
}

Derivatives::~Derivatives()
{
}

bool Derivatives::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShAttrib1f SH_DECL(scale_x) = 1.0;
  scale_x.range(0.0, 10.0);
  ShAttrib1f SH_DECL(scale_y) = 0.0;
  scale_y.range(0.0, 10.0);

  ShImage image;
  image.loadPng(normalize_path(SHMEDIA_DIR "/textures/rgby.png"));
  ShTable2D<ShColor3fub> texture(image.width(), image.height());
  texture.memory(image.memory());
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f i;
    ShOutputColor3f o;

    ShColor3f t = texture(i);
    
    o = abs(scale_x * dx(t)) + abs(scale_y * dy(t));
  } SH_END_PROGRAM;
  return true;
}


Derivatives Derivatives::instance = Derivatives();

