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
  image.loadPng(filename.c_str());
  LinInterp<LargeTexture<ShUnclamped<ShTextureRect<ShColor4f> >, 4, 2 > > Img(image.width(), image.height());
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
