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
  load_PNG(test_image, normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png"));

  ShTableCube<ShColor4fub> cubemap(test_image.width(), test_image.height());
  cubemap.name("cubemap");
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      load_PNG(image, normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png"));
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
    refrv = refract(-viewv,onorm,ShAttrib1f(1.0/eta)); // Compute refraction vector
    fres = fresnel(viewv,onorm,ShAttrib1f(eta)); // Compute fresnel term

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
