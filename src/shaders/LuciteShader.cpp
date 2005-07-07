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
  test_image.loadPng(normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png"));

  ShTableCube<ShColor4fub> SH_DECL(cubemap) =
    ShTableCube<ShColor4fub>(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png"));
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
