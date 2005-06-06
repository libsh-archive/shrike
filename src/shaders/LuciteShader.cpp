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

#include "util.hpp"

class LuciteShader : public Shader {
public:
  ShProgram vsh, fsh;

  LuciteShader();
  ~LuciteShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

LuciteShader::LuciteShader()
  : Shader("Refraction: Lucite")
{
}

LuciteShader::~LuciteShader()
{
}

bool LuciteShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  test_image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png");

  ShTextureCube<ShColor4fub> SH_DECL(cubemap) =
    ShTextureCube<ShColor4fub>(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  ShAttrib3f SH_DECL(eta) = ShAttrib3f(1.32f,1.3f,1.28f);
  eta.title("relative indices of refraction");
  eta.description("relative indices of refraction for each color component to model dispersion");
  eta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f SH_DECL(ipos);
    ShInputNormal3f SH_DECL(inorm);
    
    ShOutputPosition4f SH_DECL(opos);    // Position in NDC
    ShOutputNormal3f SH_DECL(onorm);     // view-space normal
    ShOutputVector3f SH_DECL(reflv);     // reflection vector
    ShOutputVector3f refrv[3];  // refraction vectors (per RGB channel)
    refrv[0].name("refrv[0]");  // should be nicer way to name these
    refrv[1].name("refrv[1]");
    refrv[2].name("refrv[2]");
    ShOutputAttrib3f SH_DECL(fres);      // fresnel terms (per RGB channel)

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f SH_DECL(posv) = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f SH_DECL(viewv) = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector

    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;

    for (int i=0; i<3; i++) {
    	refrv[i] = refract(-viewv,onorm,1.0/eta[i]); // Compute refraction vectors

        // actually do refraction lookup in model space
        refrv[i] = Globals::mv_inverse | refrv[i];

        fres[i] = fresnel(viewv,onorm,eta[i]); // Compute fresnel term
    }
  } SH_END;
  vsh.name("LuciteShader::vsh");
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f SH_DECL(posh);
    ShInputNormal3f SH_DECL(n);  // normal
    ShInputVector3f SH_DECL(reflv);     // reflection vector
    ShInputVector3f refrv[3];     // refraction vectors (per RGB channel)
    refrv[0].name("refrv[0]");  // should be nicer way to name these
    refrv[1].name("refrv[1]");
    refrv[2].name("refrv[2]");
    ShInputAttrib3f SH_DECL(fres);         // fresnel terms (per RGB channel)

    ShOutputColor3f SH_DECL(result);
    
    result = fres*cubemap(reflv)(0,1,2);
    for (int i=0; i<3; i++) {
        result[i] += (1.0f-fres[i])*cubemap(refrv[i])(i); 
    }
  } SH_END;
  fsh.name("LuciteShader::fsh");

  return true;
}


LuciteShader the_lucite_shader;
