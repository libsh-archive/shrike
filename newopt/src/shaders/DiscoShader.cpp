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

class DiscoShader : public Shader {
public:
  DiscoShader();
  ~DiscoShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

DiscoShader::DiscoShader()
  : Shader("Animation: Cellnoise Disco Shader")
{
}

DiscoShader::~DiscoShader()
{
}

bool DiscoShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  test_image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png");

  ShTextureCube<ShColor4f> cubemap(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("texcoord", "viewVec", "normal", "halfVec", "lightVec", "posh") << vsh;

  // find reflected vector for cube mapping later
  ShProgram reflector = SH_BEGIN_PROGRAM() {
    ShInputVector3f SH_DECL(viewVec);
    ShOutputVector3f SH_DECL(reflectVec);
    ShInOutNormal3f SH_DECL(normal);
    reflectVec = Globals::mv_inverse | ShVector3f(2.0f * dot(normal, viewVec) * normal - viewVec); 
  } SH_END;
  vsh = namedConnect(vsh, reflector, true); 

  ShAttrib1f SH_DECL(time) = 0.0;
  time.range(0.0f, 4.0f); 

  ShAttrib1f SH_DECL(tileFrequency) = 16.0;
  tileFrequency.range(0.0, 128.0);

  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShAttrib1f SH_DECL(envAmount) = ShAttrib1f(0.5);

  ShColor3f SH_DECL(defaultColor) = ShConstAttrib3f(0.25f, 0.25f, 0.25f);
  ShAttrib1f SH_DECL(texscale) = 1.0;

  ShProgram discoTiler = SH_BEGIN_PROGRAM() {
    ShInputTexCoord2f SH_DECL(texcoord);
    ShInputVector3f SH_DECL(reflectVec);
    ShOutputColor3f SH_DECL(kd);
    ShOutputColor3f SH_DECL(ks);

    // Translate those texcoords a bit
    texcoord(0) = texcoord(0) + time * texscale;
    
    ShAttrib3f p;
    p(0,1) = texcoord * tileFrequency; 
    p(2) = 0.0f; // use z for the time parameter

    // use cellnoise to decide when to switch colours on a cell
    ShAttrib1f timestep = cellnoise<1>(p);
    ShAttrib1f timeoffset = time / timestep;
    p(2) = timeoffset;

    // use cellnoise to find colour of a cell and use power to make 
    // colours funkier (and maybe add environment mapping sweetness?)
    kd = 3.0f * cellnoise<3>(p);
    kd(0) = pow(kd(0), ShConstAttrib1f(2.0f));
    kd(1) = pow(kd(1), ShConstAttrib1f(2.0f));
    kd(2) = pow(kd(2), ShConstAttrib1f(2.0f));

    // make little circles...
    p = frac(p) - ShConstAttrib3f(0.5f, 0.5f, 0.0f); 
    kd = lerp( (dot(p(0,1), p(0,1)) < 0.25), kd, defaultColor);

    kd = cubemap(reflectVec)(0,1,2) * kd; 
    ks = kd;
  } SH_END;
  
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::phong<ShColor3f>() << shExtract("specExp") << exponent; 
  fsh = fsh << shExtract("irrad") << lightColor;
  fsh = fsh << discoTiler;
  return true;
}


DiscoShader the_disco_shader;