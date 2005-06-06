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
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"
#include "ClipMap.hpp"

using namespace SH;
using namespace ShUtil;

#define SIZE_IMAGE 1024

class ClipMapShader : public Shader {
public:
  ClipMapShader();
  ~ClipMapShader();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  std::string fcenter;

  static ClipMapShader instance;

private:
  bool textInit;
  ClipMap<ShTextureRect<ShColor4fub>, SIZE_IMAGE > Img;
};

ClipMapShader::ClipMapShader()
  : Shader("Textures: Clip Map"), fcenter("512:512")
{
  setStringParam("view center", fcenter);
  textInit = false;
}

ClipMapShader::~ClipMapShader()
{
}

bool ClipMapShader::init()
{
  if(!textInit) {
    ShImage image;
    std::string filename = SHMEDIA_DIR "/largetextures/earth.png";
    image.loadPng(filename.c_str());
    Img = ClipMap<ShTextureRect<ShColor4fub>, SIZE_IMAGE>(image.width(), image.height());
    Img.internal(true);
    Img.memory(image.memory());
    textInit = true;
  }
  int center[2];
  sscanf(fcenter.c_str(),"%i:%i",&(center[0]),&(center[1]));
  Img.setCenter(center);

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

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
     
    ShOutputColor3f result = Img(tc)(0,1,2);
  } SH_END;
  return true;
}

ClipMapShader ClipMapShader::instance = ClipMapShader();
