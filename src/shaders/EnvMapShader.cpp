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

#include "util.hpp"

class EnvMapShader : public Shader {
public:
  ShProgram vsh, fsh;

  EnvMapShader();
  ~EnvMapShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

EnvMapShader::EnvMapShader()
  : Shader("Reflection: Mirror")
{
}

EnvMapShader::~EnvMapShader()
{
}

bool EnvMapShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  test_image.loadPng(normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png"));

  ShTableCube<ShColor4fub> cubemap(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png"));
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputVector3f reflv; // reflection vector

    opos = Globals::mvp | ipos; // Compute NDC position
    ShNormal3f onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector
    reflv = reflect(viewv,onorm); // Compute reflection vector

    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputVector3f reflv;

    ShOutputColor3f result;
    
    result = cubemap(reflv)(0,1,2); 
  } SH_END;

  return true;
}

EnvMapShader the_envmap_shader;
