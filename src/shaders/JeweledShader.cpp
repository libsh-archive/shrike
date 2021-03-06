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
// multiple homomorphic materials blended with a material map
#include <sh/sh.hpp>
#include <shutil/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class JeweledShader : public Shader {
public:
  JeweledShader(const Globals&);
  ~JeweledShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

JeweledShader::JeweledShader(const Globals& globals)
  : Shader("Homomorphic Factorization: Material Mapping", globals)
{
}

JeweledShader::~JeweledShader()
{
}

bool JeweledShader::init()
{
  ShAttrib2f texture_scale(1.0,1.0);
  texture_scale.name("Texture Scale");
  texture_scale.range(0.0,10.0);

  ShAttrib1f theta(1.5);
  theta.name("Index of Refraction");
  theta.range(0.0,3.0);

  ShAttrib2f threshold(0.45,0.55);
  threshold.name("Texture thresholds");
  threshold.range(0.0,1.0);

  ShAttrib1f width(0.01);
  width.name("Texture transition width");
  width.range(0.0,0.2);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;// model-space position
    ShInputNormal3f inorm; // model-space normal
    ShInputTexCoord2f tc;  // texture coordinate (passed through)
    ShInputVector3f itan;  // model-space tangent
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputVector3f light;  // direction to light (surface frame)
    ShOutputVector3f view;   // direction to eye (surface frame)
    ShOutputTexCoord2f otc;  // tex coords passed through
    ShOutputVector3f reflv;  // reflection vector
    ShOutputAttrib1f fres;   // fresnel term

    otc = tc * texture_scale;
    opos = m_globals.mvp | ipos; // Compute NDC position
    ShNormal3f n = m_globals.mv | inorm; // Compute view-space normal
    ShNormal3f t = m_globals.mv | itan; // Compute view-space normal
    n = normalize(n);
    t = normalize(t);

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f lightv = normalize(m_globals.lightPos - posv); // Compute light direction
    ShVector3f viewv = -normalize(posv); // Compute view vector
    reflv = reflect(viewv,n);  // view-space reflection vector
    reflv = m_globals.mv_inverse | reflv;  // do env map lookup in model space
    fres = fresnel(viewv,n,theta);

    // compute local surface frame (in view space)
    // ShVector3f t = normalize(itan - (itan|n)*n);
    ShVector3f s = normalize(cross(t,n));
    
    // project view and light vectors onto local surface frame
    view(0) = (t|viewv);
    view(1) = (s|viewv);
    view(2) = (n|viewv);

    light(0) = (t|lightv);
    light(1) = (s|lightv);
    light(2) = (n|lightv);  // if positive, is irradiance scale
  } SH_END;

  ShImage image;

#define NMATS 3
#define LMAT 0
#define UMAT 2
  ShTable2D<ShColor3fub> ptex[NMATS];
  ShTable2D<ShColor3fub> qtex[NMATS];

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/mystique/mystique64_0.png"));
  ptex[0].size(image.width(), image.height());
  ptex[0].memory(image.memory());
  ptex[0].name("Mystique p texture");

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/mystique/mystique64_1.png"));
  qtex[0].size(image.width(), image.height());
  qtex[0].memory(image.memory());
  qtex[0].name("Mystique q texture");

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/satin/satinp.png"));
  ptex[1].size(image.width(), image.height());
  ptex[1].memory(image.memory());
  ptex[1].name("Satin p texture");

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/satin/satinq.png"));
  qtex[1].size(image.width(), image.height());
  qtex[1].memory(image.memory());
  qtex[1].name("Satin q texture");

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_0.png"));
  ptex[2].size(image.width(), image.height());
  ptex[2].memory(image.memory());
  ptex[2].name("Garnet red p texture");

  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_1.png"));
  qtex[2].size(image.width(), image.height());
  qtex[2].memory(image.memory());
  qtex[2].name("Garnet red q texture");

  // Specular highlight (to be added when needed...)
  load_PNG(image, normalize_path(SHMEDIA_DIR "/brdfs/specular.png"));
  ShTable2D<ShColor3fub> stex(image.width(), image.height());
  stex.memory(image.memory());
  stex.name("Specular highlight texture");
  
  // Cube map for mirror reflection
  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShTableCube<ShColor4fub> env;
  env.name("Environment map");
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      load_PNG(image, normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png"));
      env.size(image.width(), image.height());
      env.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }
  
  ShWrapRepeat< ShTable2D<ShColor3fub> > mat;

  // Material map (threshold based...)
  load_PNG(image, normalize_path(SHMEDIA_DIR "/textures/halftone.png"));
  mat.size(image.width(), image.height());
  mat.memory(image.memory());
  mat.name("Filgiree distance map");
  
  // Scale factors
  ShColor3f alpha[NMATS];
  ShAttrib1f diffuse[NMATS];
  ShAttrib1f specular[NMATS];
  ShAttrib1f mirror[NMATS];

  // for mystique
  alpha[0] = ShColor3f(0.0011786,0.165452,0.0338959);
  alpha[0].name("Mystique correction color");
  diffuse[0] = ShAttrib1f(10.0);
  diffuse[0].range(0.0,20.0);
  diffuse[0].name("Mystique diffuse scale");
  specular[0] = ShAttrib1f(0.3);
  specular[0].range(0.0,1.0);
  specular[0].name("Mystique specular scale");
  mirror[0] = ShAttrib1f(0.07);
  mirror[0].range(0.0,1.0);
  mirror[0].name("Mystique mirror scale");

  // for satin
  alpha[1] = ShColor3f(0.762367,0.762367,0.762367);
  alpha[1].name("Satin correction color");
  diffuse[1] = ShAttrib1f(1.0);
  diffuse[1].range(0.0,5.0);
  diffuse[1].name("Satin diffuse scale");
  specular[1] = ShAttrib1f(0.0);
  specular[1].range(0.0,1.0);
  specular[1].name("Satin specular scale");
  mirror[1] = ShAttrib1f(0.05);
  mirror[1].range(0.0,1.0);
  mirror[1].name("Satin mirror scale");

  // for garnet red
  alpha[2] = ShColor3f(0.0410592,0.0992037,0.0787714);
  alpha[2].name("Garnet red correction color");
  diffuse[2] = ShAttrib1f(5.0);
  diffuse[2].range(0.0,20.0);
  diffuse[2].name("Garnet red diffuse scale");
  specular[2] = ShAttrib1f(0.4);
  specular[2].range(0.0,1.0);
  specular[2].name("Garnet red specular scale");
  mirror[2] = ShAttrib1f(0.09);
  mirror[2].range(0.0,1.0);
  mirror[2].name("Garnet red mirror scale");

  // Light parameters
  ShColor3f light_color = ShColor3f(1.0,1.0,1.0);
  light_color.range(0.0,1.0);
  light_color.name("Light color");
  ShAttrib1f light_power = ShAttrib1f(5.0);
  light_power.range(0.0,20.0);
  light_power.name("Light power");
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputVector3f light;
    ShInputVector3f view;
    ShInputTexCoord2f u;
    ShInputVector3f reflv;  
    ShInputAttrib1f fres;  

    ShOutputColor3f result;

    // Normalize (theoretically not needed if compiler smart enough)
    light = normalize(light);
    view = normalize(view);

    // compute half vector
    ShVector3f half = normalize(view + light); 

    // Theoretically not needed if use common subexpression elimination...
    ShTexCoord2f hu = parabolic_norm(half);
    ShTexCoord2f lu = parabolic_norm(light);
    ShTexCoord2f vu = parabolic_norm(view);

    // Look up shared specular highlight component 
    ShColor3f spec = fres * stex(hu);
    // Look up shared mirror reflection component 
    ShColor3f mirr = fres * env(reflv)(0,1,2);

    // BUG: doesn't work (satin always white) if this not here...
    // in theory, should not be needed, bug in compiler?
    result = ShColor3f(0.0,0.0,0.0);
    
    // threshold the filgiree distance texture
    ShAttrib1f m = mat(u)(0);
    ShAttrib3f mask;
    mask(1) = linstep(m,threshold(0),width);
    mask(2) = linstep(m,threshold(1),width);
    mask(0) = ShAttrib1f(1.0) - mask(1);
    mask(1) = mask(1) - mask(2);

    // sum contributions of all materials
    for (int i=LMAT; i<=UMAT; i++) {
       ShColor3f f;

       // Incorporate diffuse scale, correction factor, and irradiance
       f = diffuse[i] * alpha[i] * pos(light(2));

       // do texture lookups
       f *= ptex[i](lu);
       f *= qtex[i](hu);
       f *= ptex[i](vu);

       // Add in specular highlight term (should use Fresnel here)
       f += specular[i] * spec * pos(light(2));
       // Add in mirror term (should use Fresnel here) 
       f += mirror[i] * mirr;

       //f = ShColor3f(i == 0 ? 1.0 : 0.0, i == 1 ? 1.0 : 0.0, i == 2 ? 1.0 : 0.0);
       // mask by material map and fresnel term
       f *= (1.0f - fres);
       f *= mask(i);

       // accumulate
       result += f;
    }
    // Take into account light power and colour
    result *= light_power * light_color;
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new JeweledShader(globals));
    return list;
  }
}
#else
static StaticLinkedShader<JeweledShader> instance = 
       StaticLinkedShader<JeweledShader>();
#endif
