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
#include <list>
#include "Shader.hpp"
#include "Globals.hpp"
#include "ShrikeCanvas.hpp"

using namespace SH;
using namespace ShUtil;
#include "util.hpp"

// VCS direction and up
ShVector3f lightDir;
ShVector3f lightUp;
ShAttrib1f width;
ShAttrib1f invwidth;
ShAttrib1f height;
ShAttrib1f invheight;

class AlgebraWrapper: public Shader {
public:
  AlgebraWrapper(std::string name, int lightidx, int surfmapidx, int surfidx, int postidx) 
    : Shader(name), lightidx(lightidx), surfmapidx(surfmapidx), surfidx(surfidx), postidx(postidx) {}

  bool init(); 
  void render();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:
  int lightidx, surfmapidx, surfidx, postidx;
  ShProgram vsh, fsh;
};

class AlgebraShaders {
public:
  AlgebraShaders();
  ~AlgebraShaders();

  static bool init_all();

private:
  friend class AlgebraWrapper;

  static const int LIGHT = 3;
  static const int SURFMAP = 2;
  static const int SURFACE = 7;
  static const int POST = 3;

  static ShProgram lightsh[LIGHT];
  static const char* lightName[LIGHT];

  static ShProgram surfmapsh[SURFMAP];
  static const char* surfmapName[SURFMAP];

  static ShProgram surfsh[SURFACE];
  static const char* surfName[SURFACE];

  static ShProgram postsh[POST];
  static const char* postName[POST];

  typedef std::list<Shader*> ShaderList;
  static std::list<Shader*> shaders; 
  static bool doneInit;
};

AlgebraShaders::ShaderList AlgebraShaders::shaders;
ShProgram AlgebraShaders::lightsh[AlgebraShaders::LIGHT];
ShProgram AlgebraShaders::surfmapsh[AlgebraShaders::SURFMAP];
ShProgram AlgebraShaders::surfsh[AlgebraShaders::SURFACE];
ShProgram AlgebraShaders::postsh[AlgebraShaders::POST];
bool AlgebraShaders::doneInit = false;

const char* AlgebraShaders::lightName[] = {
  "Point Light",
  "Spot Light",
  "Textured Light"
};

const char*  AlgebraShaders::surfmapName[] = {
  0,
  "Bump Map"
};

const char* AlgebraShaders::surfName[] = {
  0,
  "Diffuse",
  //  "Specular Surface",
  //  "Phong Surface",
  "Textured Phong",
  "Procedural Worley Phong",
  "Gooch",
  "Satin Homomorphic BRDF",
  "Ashikhmin"
};

const char* AlgebraShaders::postName[] = {
  0,
  "Halftone PostOp",
  "Noisify PostOp"
};

bool AlgebraWrapper::init() {
  AlgebraShaders::init_all();
  ShProgram lightsh = AlgebraShaders::lightsh[lightidx];
  ShProgram surfmapsh = AlgebraShaders::surfmapsh[surfmapidx];
  ShProgram surfsh = AlgebraShaders::surfsh[surfidx];
  ShProgram postsh = AlgebraShaders::postsh[postidx];

  fsh = namedCombine(lightsh, surfmapsh);
  fsh = namedConnect(fsh, surfsh);
  fsh = namedConnect(fsh, postsh);

  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp, 1);
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = namedAlign(vsh, fsh);
  return true;
}

void AlgebraWrapper::render() {
  lightDir = -normalize(Globals::mv | Globals::lightDirW); 
  ShVector3f horiz = cross(lightDir, ShConstVector3f(0.0f, 1.0f, 0.0f));
  lightUp = cross(horiz, lightDir);

  const ShrikeCanvas *canvas = ShrikeCanvas::instance();
  width = canvas->GetClientSize().GetWidth();
  invwidth = 1.0f / width;
  height = canvas->GetClientSize().GetHeight();
  invheight = 1.0f / height;

  // set up lighting crap
  Shader::render();
}


AlgebraShaders::AlgebraShaders()
{

  for(int i = 0; i < LIGHT; ++i) {
    for(int j = 0; j < SURFMAP; ++j) {
      for(int k = 0; k < SURFACE; ++k) {
        for(int l = 0; l < POST; ++l) {
          
          std::string name = std::string("Algebra");
          if (lightName[i]) {name += ": "; name += lightName[i];}
          if (surfmapName[j]) {name += ": "; name += surfmapName[j];}
          if (surfName[k]) {name += ": "; name += surfName[k];}
          if (postName[l]) {name += ": "; name += postName[l];}
          
          shaders.push_back(new AlgebraWrapper(name, i, j, k, l)); 
        }
      }
    }
  }
}

AlgebraShaders::~AlgebraShaders()
{
  for(ShaderList::iterator I = shaders.begin(); I != shaders.end(); ++I) {
    delete *I;
  }
}

// returns a KernelSurface::phong shader with kd filled in by a worley shader
ShProgram worleySurface() {
  ShAttrib3f SH_NAMEDECL(color1, "Worley Color1") = ShColor3f(3.0, 0.75, 0.0);
  color1.range(-3.0f, 3.0f);
  ShAttrib3f SH_NAMEDECL(color2, "Worley Color2") =  ShColor3f(0.0f, 0.0f, 0.0f);
  color2.range(-3.0f, 3.0f);
  ShAttrib4f SH_NAMEDECL(coeff, "Worley Coefficients") = ShConstAttrib4f(2.5, -0.5f, -0.1f, 0);
  coeff.range(-3.0f, 3.0f);
  ShAttrib1f SH_NAMEDECL(freq, "Worley Frequency") = ShConstAttrib1f(16.0f);
  freq.range(0.1f, 64.0f);

  ShProgram worleysh = shWorley<4, 2, float>(false); // pass in coefficient
  worleysh = (dot<ShAttrib4f>() << coeff) << worleysh; 

  ShProgram scaler = SH_BEGIN_PROGRAM() {
    ShInOutTexCoord2f SH_DECL(texcoord) = freq * texcoord;
  } SH_END; 

  worleysh = worleysh << scaler; 

  ShProgram clamper = SH_BEGIN_PROGRAM() {
    ShInOutAttrib1f SH_DECL(scalar) = clamp(scalar, 0.0f, 1.0f);
  } SH_END;

  worleysh = clamper << worleysh;

  ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
  return ShKernelSurface::phong<ShColor3f>() << colorsh << worleysh;
}

ShProgram satinSurface() {
  ShImage image;

  // TODO: should have array of available BRDFs with correction
  // factor for each, hidden uniforms (don't want user to play with
  // alpha, really), pulldown menu to select BRDFs from list,
  // settings for extra specularities, etc. etc.
  image.loadPng(SHMEDIA_DIR "/brdfs/satin/satinp.png");
  ShTexture2D<ShColor3f> ptex(image.width(), image.height());
  ptex.internal(true);
  ptex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/satin/satinq.png");
  ShTexture2D<ShColor3f> qtex(image.width(), image.height());
  qtex.internal(true);
  qtex.memory(image.memory());

  // HACK, satin doesn't have specular part, turned off by default
  image.loadPng(SHMEDIA_DIR "/textures/ks.png");
  ShTexture2D<ShColor3f> stex(image.width(), image.height());
  stex.name("Satin Texture");
  stex.memory(image.memory());

  // these scale factors are specific to satin
  ShColor3f SH_DECL(alpha) = ShColor3f(0.762367,0.762367,0.762367);
  ShAttrib1f SH_DECL(diffuse) = ShAttrib1f(1.0);
  diffuse.range(0.0,5.0);
  ShAttrib1f SH_DECL(specular) = ShAttrib1f(0.0);
  specular.range(0.0,1.0);
  ShAttrib1f SH_DECL(light_power) = ShAttrib1f(1.0);
  light_power.range(0.0,100.0);

  ShProgram fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputColor3f SH_DECL(irrad);
    ShInputVector3f SH_DECL(lightVect);
    ShInputVector3f SH_DECL(halfVect);
    ShInputVector3f SH_DECL(viewVect);
    ShInputPosition4f SH_DECL(posh);

    ShOutputColor3f SH_DECL(result);

    // Normalize (theoretically not needed if compiler smart enough)
    lightVect = normalize(lightVect);
    viewVect = normalize(viewVect);
    halfVect = normalize(halfVect);

    // Incorporate diffuse scale, correction factor, and irradiance
    result = diffuse * alpha * pos(lightVect(2));

    // Theoretically not needed if use common subexpression elimination...
    ShTexCoord2f hu = parabolic_norm(halfVect);
    ShTexCoord2f lu = parabolic_norm(lightVect);
    ShTexCoord2f vu = parabolic_norm(viewVect);

    // TODO: SHOULD use automatic projective normalization in texture lookup...
    // and/or parabolic texture type instead.
    result *= ptex(lu);
    result *= qtex(hu);
    result *= ptex(vu);

    // Add in specular term (also represented using parabolic map)
    result += specular * stex(hu) * pos(lightVect(2));

    // Take into account lightVect power and colour
    result *= light_power * irrad; 
  } SH_END;
  return fsh;
}

namespace {
/* Ashikhmin Surface Atom from Stefanus' demo...
 * Move some of this into the utils library later */
template<int N>
ShGeneric<N, float> pow5(const ShGeneric<N, float>& f)
{
  ShAttrib<N, SH_TEMP, float> t =  f * f;
  return t * t * f;
}

ShColor3f schlick(ShColor3f refl, ShAttrib1f kh)
{
  return refl + (ShColor3f(1.0, 1.0, 1.0) - refl)*pow5(1.0f - kh);
}

ShColor3f ashikhmin_specular(ShAttrib1f nu, ShAttrib1f nv,
			     ShNormal3f n, ShVector3f h,
			     ShVector3f light, ShVector3f viewer,
			     ShVector3f u, ShVector3f v,
			     ShColor3f refl)
{
  ShVector3f k = viewer; // either light or viewer works here

#define CLAMP(x) max(x, 0.01)

  ShAttrib1f hn = CLAMP(h|n);
  ShAttrib1f kn = CLAMP(k|n);
  ShAttrib1f ln = CLAMP(light|n);
  ShAttrib1f vn = CLAMP(viewer|n);
  ShAttrib1f kh = CLAMP(k|h);
  ShAttrib1f hu = (h|u);
  ShAttrib1f hv = (h|v);

  ShAttrib1f scale = sqrt((nu + 1.0f) * (nv + 1.0f))/(8.0*M_PI);
  ShAttrib1f exponent = (nu*hu*hu + nv*hv*hv)/(1.0f - hn*hn);
  ShAttrib1f geom = pow(hn, exponent)/(kn*max(ln, vn));

  return scale * geom * schlick(refl, kh);
}

ShColor3f ashikhmin_diffuse(ShNormal3f normal,
			    ShVector3f light, ShVector3f viewer,
			    ShColor3f spec, ShColor3f diffuse)
{
  ShColor3f scale = (28.0/(23.0*M_PI))*diffuse*(ShColor3f(1.0, 1.0, 1.0) - spec);

  ShAttrib1f v = 1.0f - pow5(1.0f - max(normal|light, 0.0)/2.0f);
  ShAttrib1f l = 1.0f - pow5(1.0f - max(normal|viewer, 0.0)/2.0f);

  return scale * v * l;
}

ShColor3f ashikhmin(ShAttrib1f nu, ShAttrib1f nv,
		    ShNormal3f n, ShVector3f h,
		    ShVector3f light, ShVector3f viewer,
		    ShVector3f u, ShVector3f v,
		    ShColor3f spec, ShColor3f diffuse)
{ 
  return ashikhmin_specular(nu, nv, n, h, light, viewer, u, v, spec)
    + ashikhmin_diffuse(n, light, viewer, spec, diffuse);
}
}


ShProgram ashikhminSurface() {
  ShColor3f SH_DECL(diffuse) = ShColor3f(0.0, 1.0, 0.5);
  ShColor3f SH_DECL(specular) = ShColor3f(1.0, .5, 0.8)/20.0f;
  ShColor3f SH_DECL(ambient) = ShColor3f(0.0, 0.1, 0.05);
  ShAttrib1f SH_DECL(nu) = 1000.0f;
  ShAttrib1f SH_DECL(nv) = 10.0f;
  specular.range(0.0f, 0.05f);
  nu.range(10.0f, 10000.0f);
  nv.range(10.0f, 10000.0f);
  
  ShProgram fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputColor3f SH_DECL(irrad);
    ShInputNormal3f SH_DECL(normal);
    ShInputVector3f SH_DECL(viewVec);
    ShInputVector3f SH_DECL(halfVec);
    ShInputVector3f SH_DECL(lightVec);
    ShInputVector3f SH_DECL(tangent);
    ShInputPosition4f SH_DECL(posh);

    ShOutputColor3f SH_DECL(result);
    ShVector3f tangent2 = cross(normal, tangent);
    result = ashikhmin(nu, nv, normalize(normal), normalize(halfVec), normalize(lightVec),
		       normalize(viewVec),
		       tangent, tangent2, 
		       specular, diffuse) * irrad
    + ambient;
  } SH_END_PROGRAM;
  return fsh;
}

bool AlgebraShaders::init_all()
{

  if( doneInit ) return true; 
  int i;
  doneInit = true;

  ShImage image;

  // useful globals
  ShColor3f SH_NAMEDECL(lightColor, "Light Colour") = ShConstColor3f(1.0f, 1.0f, 1.0f);
  ShAttrib1f SH_NAMEDECL(falloff, "Light falloff angle") = ShConstAttrib1f(0.35f); 
  falloff.range(0.0f, M_PI);
  ShAttrib1f SH_NAMEDECL(lightAngle, "Light cutoff angle") = ShConstAttrib1f(0.5f);
  lightAngle.range(0.0f, M_PI);

  // TODO handle lightDirection and light up properly
  ShColor3f SH_NAMEDECL(kd, "Diffuse Color") = ShConstColor3f(1.0f, 0.5f, 0.7f);
  ShColor3f SH_NAMEDECL(ks, "Specular Color") = ShConstColor3f(0.7f, 0.7f, 0.7f);
  ShColor3f SH_NAMEDECL(cool, "Cool Color") = ShConstColor3f(0.4f, 0.4f, 1.0f);
  ShColor3f SH_NAMEDECL(warm, "Warm Color") = ShConstColor3f(1.0f, 0.4f, 0.4f);
  ShAttrib1f SH_NAMEDECL(specExp, "Specular Exponent") = ShConstAttrib1f(48.13f);
  specExp.range(0.0f, 256.0f);

  // ****************** Make light shaders
  i = 0;
  lightsh[i++] = ShKernelLight::pointLight<ShColor3f>() << lightColor;
  lightsh[i++] = ShKernelLight::spotLight<ShColor3f>() << lightColor << falloff << lightAngle << lightDir;


  ShAttrib1f SH_NAMEDECL(texLightScale, "Mask Scaling Factor") = ShConstAttrib1f(5.0f);
  texLightScale.range(1.0f, 10.0f);
  image.loadPng(SHMEDIA_DIR "/mats/inv_oriental038.png");
  ShTexture2D<ShColor3f> lighttex(image.width(), image.height());
  lighttex.memory(image.memory());
  lightsh[i++] = ShKernelLight::texLight2D(lighttex) << texLightScale << lightAngle << lightDir << lightUp;

  //lightsh[1] = ShKernelLight::spotLight(ShColor3f>() << lightColor << falloff << lightAngle << lightDir;
  //lightName[1] = "Spot Light";
  
  // ****************** Make bump/frame mapping shaders 
  i = 0;
  surfmapsh[i++] = keep<ShNormal3f>("normal"); 

  image.loadPng(SHMEDIA_DIR "/bumpmaps/bumps_normals.png");
  ShTexture2D<ShColor3f> normaltex(image.width(), image.height());
  normaltex.name("Bumpmap Normals");
  normaltex.memory(image.memory());

  ShAttrib1f SH_NAMEDECL(bumpScale, "Bump Scaling Factor") = ShConstAttrib1f(1.0f);
  bumpScale.range(0.0f, 10.0f);

  // make a VCS bump mapper by reading in normal map and extracting gradients
  surfmapsh[i] = SH_BEGIN_PROGRAM() {
    ShInputTexCoord2f SH_DECL(texcoord);
    ShOutputAttrib2f SH_DECL(gradient);
    ShColor3f norm = normaltex(texcoord) - ShConstAttrib3f(0.5, 0.5, 0.0);
    gradient = norm(0,1) * bumpScale;
  } SH_END;
  surfmapsh[i] = ShKernelSurfMap::vcsBump() << surfmapsh[i];
  i++;
  
  // ****************** Make surface shaders 
  i = 0;
  surfsh[i++] = ShKernelSurface::null<ShColor3f>(); 
  surfsh[i++] = ShKernelSurface::diffuse<ShColor3f>() << kd;
  //surfsh[i++] = ShKernelSurface::specular<ShColor3f>() << ks << specExp;
  //surfsh[i++] = ShKernelSurface::phong<ShColor3f>() << kd << ks << specExp;

  image.loadPng(SHMEDIA_DIR "/textures/rustkd.png");
  ShTexture2D<ShColor3f> difftex(image.width(), image.height());
  difftex.name("Diffuse texture");
  difftex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/textures/rustks.png");
  ShTexture2D<ShColor3f> spectex(image.width(), image.height());
  spectex.name("Specular texture");
  spectex.memory(image.memory());

  surfsh[i] = ShKernelSurface::phong<ShColor3f>() << ( access(difftex) & access(spectex) );
  surfsh[i] = surfsh[i] << shExtract("specExp") << specExp;
  i++;

  surfsh[i] =  worleySurface() << shExtract("ks") << ks; 
  surfsh[i] = surfsh[i] << shExtract("specExp") << specExp;
  i++;

  surfsh[i++] = ShKernelSurface::gooch<ShColor3f>() << kd << cool << warm; 

  surfsh[i++] =  satinSurface(); 
  surfsh[i++] = ashikhminSurface();

  // ******************* Make postprocessing shaders
  i = 0;
  image.loadPng(SHMEDIA_DIR "/textures/halftone.png");
  ShTexture2D<ShColor3f> halftoneTex(image.width(), image.height());
  halftoneTex.name("Halftoning texture");
  halftoneTex.memory(image.memory());


  postsh[i++] = keep<ShColor3f>("result"); 

  ShAttrib1f SH_NAMEDECL(htscale, "Scaling Factor") = ShConstAttrib1f(10.0f);
  htscale.range(1.0f, 400.0f);
  ShProgram scaler = SH_BEGIN_PROGRAM() {
    ShInputPosition4f SH_DECL(posh);
    ShOutputTexCoord2f SH_DECL(texcoord) = posh(0,1) * invheight * htscale;
    //ShInOutTexCoord2f SH_DECL(texcoord) *= htscale; 
  } SH_END;
  postsh[i++] = namedConnect(scaler, shHalftone<ShColor3f>(halftoneTex));
  
  ShAttrib1f SH_NAMEDECL(nscale, "Scaling Factor") = ShConstAttrib1f(50.0f);
  nscale.range(1.0f, 400.0f);

  ShAttrib1f SH_NAMEDECL(noiseScale, "Noise Amount") = ShConstAttrib1f(0.2f);
  noiseScale.range(0.0f, 1.0f);
  scaler = SH_BEGIN_PROGRAM() {
    ShInputPosition4f SH_DECL(posh);
    ShOutputTexCoord2f SH_DECL(texcoord) = posh(0,1) * invheight * nscale;
  } SH_END;
  // replace texcoord with scaled posh
  ShProgram noisifier = shNoisify<2, ShColor3f>(false) & lose<ShTexCoord2f>("texcoord"); 
  noisifier = noisifier << noiseScale;
  postsh[i++] = namedConnect(scaler, noisifier); 

  return true;
}

AlgebraShaders the_algebra_shader;

