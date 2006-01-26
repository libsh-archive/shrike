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

class Derivatives : public Shader {
public:
  Derivatives(const Globals&);
  ~Derivatives();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

Derivatives::Derivatives(const Globals& globals)
  : Shader("Debugging: Derivatives", globals)
{
}

Derivatives::~Derivatives()
{
}

bool Derivatives::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( m_globals.mv, m_globals.mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShAttrib1f SH_DECL(scale_x) = 1.0;
  scale_x.range(0.0, 10.0);
  ShAttrib1f SH_DECL(scale_y) = 0.0;
  scale_y.range(0.0, 10.0);

  ShImage image;
  load_PNG(image, normalize_path(SHMEDIA_DIR "/textures/rgby.png"));
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


#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new Derivatives(globals));
    return list;
  }
}
#else
static StaticLinkedShader<Derivatives> instance = 
       StaticLinkedShader<Derivatives>();
#endif

