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

#include "util.hpp"

class GlassShader : public Shader {
public:
  ShProgram vsh, fsh;

  GlassShader();
  ~GlassShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

GlassShader::GlassShader()
  : Shader("Refraction: Glass")
{
}

GlassShader::~GlassShader()
{
}

bool GlassShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  test_image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png");

  ShTextureCube<ShColor4f> cubemap(test_image.width(), test_image.height());
  cubemap.name("cubemap");
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  ShAttrib1f SH_DECL(eta) = ShAttrib1f(1.3f);
  eta.title("relative index of refraction");
  eta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f SH_DECL(ipos);
    ShInputNormal3f SH_DECL(inorm);
    
    ShOutputPosition4f SH_DECL(opos); // Position in NDC
    ShOutputNormal3f SH_DECL(onorm);  // view-space normal
    ShOutputVector3f SH_DECL(reflv); // Compute reflection vector
    ShOutputVector3f SH_DECL(refrv); // Compute refraction vector
    ShOutputAttrib1f SH_DECL(fres); // Compute fresnel term

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f SH_DECL(posv) = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f SH_DECL(viewv) = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector
    refrv = refract(viewv,onorm,eta); // Compute refraction vector
    fres = fresnel(viewv,onorm,eta); // Compute fresnel term

    // actually do reflection and refraction lookup in model space
    reflv = Globals::mv_inverse | reflv;
    refrv = Globals::mv_inverse | refrv;
  } SH_END;
  vsh.name("GlassShader::vsh");
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f SH_DECL(posh);
    ShInputNormal3f SH_DECL(n);  // normal
    ShInputVector3f SH_DECL(reflv); // Compute reflection vector
    ShInputVector3f SH_DECL(refrv); // Compute refraction vector
    ShInputAttrib1f SH_DECL(fres); // Compute fresnel term

    ShOutputColor3f SH_DECL(result);
    
    result = fres*cubemap(reflv)(0,1,2) + (1.0f-fres)*cubemap(refrv)(0,1,2); 
  } SH_END;
  fsh.name("GlassShader::fsh");

  return true;
}


GlassShader the_glass_shader;
