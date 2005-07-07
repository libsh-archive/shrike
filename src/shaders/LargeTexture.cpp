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
#include "LargeTexture.hpp"
#include "HDRInterp.hpp"

using namespace SH;
using namespace ShUtil;

class LargeTextureShader : public Shader {
public:
  LargeTextureShader();
  ~LargeTextureShader();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  std::string fname;

  static LargeTextureShader instance;
};

LargeTextureShader::LargeTextureShader()
  : Shader("Textures: Large Texture"), fname("earth.png")
{
  setStringParam("Image Name", fname);
}

LargeTextureShader::~LargeTextureShader()
{
}

bool LargeTextureShader::init()
{
  ShImage image;
  std::string filename = SHMEDIA_DIR "/largetextures/" + fname;
  image.loadPng(normalize_path(filename.c_str()));
  LinInterp<LargeTexture<ShTableRect<ShColor4f>, 4, 2> > Img(image.width(), image.height());
  Img.internal(true);
  Img.memory(image.memory());

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
} SH_END;

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
  scale.range(0.1,2.0);

  ShAttrib2f SH_DECL(translation) = ShAttrib2f(0.0,0.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
 
    ShOutputColor3f result = Img(tc*scale + translation)(0,1,2);
    
  } SH_END;
  return true;
}

LargeTextureShader LargeTextureShader::instance = LargeTextureShader();
