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

class AlgebraShader : public Shader {
public:
  AlgebraShader();
  ~AlgebraShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static bool init_all();

private:
  friend class AlgebraWrapper;

  static const int LIGHT = 3;
  static const int SURFMAP = 2;
  static const int SURFACE = 7;
  static const int POST = 2;

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

AlgebraShader::ShaderList AlgebraShader::shaders;
ShProgram AlgebraShader::lightsh[AlgebraShader::LIGHT];
ShProgram AlgebraShader::surfmapsh[AlgebraShader::SURFMAP];
ShProgram AlgebraShader::surfsh[AlgebraShader::SURFACE];
ShProgram AlgebraShader::postsh[AlgebraShader::POST];
bool AlgebraShader::doneInit = false;

const char* AlgebraShader::lightName[] = {
  "Point Light",
  "Spot Light",
  "Textured Light"
};

const char*  AlgebraShader::surfmapName[] = {
  "Identity",
  "Bump Map"
};

const char* AlgebraShader::surfName[] = {
  "Null Surface",
  "Diffuse",
//  "Specular Surface",
//  "Phong Surface",
  "Textured Phong",
  "Procedural Worley Phong",
  "Gooch",
  "Satin Homomorphic BRDF",
  "Ashikhmin"
};

const char* AlgebraShader::postName[] = {
  "Null PostOp",
  "Halftone PostOp",
};

bool AlgebraWrapper::init() {
  AlgebraShader::init_all();
  ShProgram lightsh = AlgebraShader::lightsh[lightidx];
  ShProgram surfmapsh = AlgebraShader::surfmapsh[surfmapidx];
  ShProgram surfsh = AlgebraShader::surfsh[surfidx];
  ShProgram postsh = AlgebraShader::postsh[postidx];

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
  ShVector3f horiz = cross(lightDir, ShConstant3f(0.0f, 1.0f, 0.0f));
  lightUp = cross(horiz, lightDir);

  const ShrikeCanvas *canvas = ShrikeCanvas::instance();
  width = canvas->m_width;
  invwidth = 1.0f / width;
  height = canvas->m_height;
  invheight = 1.0f / height;

  // set up lighting crap
  Shader::render();
}


AlgebraShader::AlgebraShader()
  : Shader("Algebra: Algebra Parent")
{

  for(int i = 0; i < LIGHT; ++i) {
    for(int j = 0; j < SURFMAP; ++j) {
      for(int k = 0; k < SURFACE; ++k) {
        for(int l = 0; l < POST; ++l) {
          std::string name = std::string("Algebra: ") + 
            lightName[i] + ": " + surfmapName[j] + ": " +
            surfName[k] + ": " + postName[l];
          shaders.push_back(new AlgebraWrapper(name, i, j, k, l)); 
        }
      }
    }
  }
}

AlgebraShader::~AlgebraShader()
{
  for(ShaderList::iterator I = shaders.begin(); I != shaders.end(); ++I) {
    delete *I;
  }
}

bool AlgebraShader::init() {
  // put together all the different combinations!
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 

  std::cout << "AlgebraShader::init()" << std::endl;
  fsh = surfsh[0]; 
  vsh = namedAlign(vsh, fsh);
  std::cout << "AlgebraShader::init() done" << std::endl;
  return true;
}

// returns a KernelSurface::phong shader with kd filled in by a worley shader
ShProgram worleySurface() {
    ShAttrib3f SH_NAMEDECL(color1, "Worley Color1") = ShColor3f(3.0, 0.75, 0.0);
    color1.range(-3.0f, 3.0f);
    ShAttrib3f SH_NAMEDECL(color2, "Worley Color2") =  ShColor3f(0.0f, 0.0f, 0.0f);
    color2.range(-3.0f, 3.0f);
    ShAttrib4f SH_NAMEDECL(coeff, "Worley Coefficients") = ShConstant4f(2.5, -0.5f, -0.1f, 0);
    coeff.range(-3.0f, 3.0f);
    ShAttrib1f SH_NAMEDECL(freq, "Worley Frequency") = ShConstant1f(16.0f);
    freq.range(0.1f, 64.0f);

    ShProgram worleysh = worleyProgram<4, float>(L2_SQ, false) << coeff; // pass in coefficient

    ShProgram scaler = SH_BEGIN_PROGRAM() {
      ShInOutTexCoord2f SH_DECL(texcoord) = freq * texcoord;
    } SH_END; 

    worleysh = worleysh << scaler; 
    worleysh = shDrop("gradient") << worleysh;

    ShProgram clamper = SH_BEGIN_PROGRAM() {
      ShInOutAttrib1f SH_DECL(scalar) = clamp(0.0f, 1.0f, scalar);
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
  ptex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/satin/satinq.png");
  ShTexture2D<ShColor3f> qtex(image.width(), image.height());
  qtex.memory(image.memory());

  // HACK, satin doesn't have specular part, turned off by default
  image.loadPng(SHMEDIA_DIR "/textures/ks.png");
  ShTexture2D<ShColor3f> stex(image.width(), image.height());
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
  ShVariableN<N, float> pow5(const ShVariableN<N, float>& f)
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

bool AlgebraShader::init_all()
{

  if( doneInit ) return true; 
  int i;
  doneInit = true;

  ShImage image;

  // useful globals
  ShColor3f SH_NAMEDECL(lightColor, "Light Colour") = ShConstant3f(1.0f, 1.0f, 1.0f);
  ShAttrib1f SH_NAMEDECL(falloff, "Light falloff angle") = ShConstant1f(0.35f); 
  falloff.range(0.0f, M_PI);
  ShAttrib1f SH_NAMEDECL(lightAngle, "Light cutoff angle") = ShConstant1f(0.5f);
  lightAngle.range(0.0f, M_PI);

  // TODO handle lightDirection and light up properly
  ShColor3f SH_NAMEDECL(kd, "Diffuse Color") = ShConstant3f(1.0f, 0.5f, 0.7f);
  ShColor3f SH_NAMEDECL(ks, "Specular Color") = ShConstant3f(0.7f, 0.7f, 0.7f);
  ShColor3f SH_NAMEDECL(cool, "Cool Color") = ShConstant3f(0.4f, 0.4f, 1.0f);
  ShColor3f SH_NAMEDECL(warm, "Warm Color") = ShConstant3f(1.0f, 0.4f, 0.4f);
  ShAttrib1f SH_NAMEDECL(specExp, "Specular Exponent") = ShConstant1f(48.13f);
  specExp.range(0.0f, 256.0f);

// ****************** Make light shaders
  i = 0;
  lightsh[i++] = ShKernelLight::pointLight<ShColor3f>() << lightColor;
  lightsh[i++] = ShKernelLight::spotLight<ShColor3f>() << lightColor << falloff << lightAngle << lightDir;


  ShAttrib1f SH_NAMEDECL(texLightScale, "Mask Scaling Factor") = ShConstant1f(5.0f);
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
  normaltex.memory(image.memory());

  ShAttrib1f SH_NAMEDECL(bumpScale, "Bump Scaling Factor") = ShConstant1f(1.0f);
  bumpScale.range(0.0f, 10.0f);

  // make a VCS bump mapper by reading in normal map and extracting gradients
  surfmapsh[i] = SH_BEGIN_PROGRAM() {
    ShInputTexCoord2f SH_DECL(texcoord);
    ShOutputAttrib2f SH_DECL(gradient);
    ShColor3f norm = normaltex(texcoord) - ShConstant3f(0.5, 0.5, 0.0);
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
  difftex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/textures/rustks.png");
  ShTexture2D<ShColor3f> spectex(image.width(), image.height());
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
  halftoneTex.memory(image.memory());


  postsh[i++] = keep<ShColor3f>("result"); 

  ShAttrib1f SH_NAMEDECL(htscale, "Scaling Factor") = ShConstant1f(50.0f);
  htscale.range(1.0f, 400.0f);
  postsh[i++] = ShKernelPost::halftone<ShColor3f>(halftoneTex) << (mul<ShAttrib1f>() << htscale << invheight);

  /*
  ShAttrib1f SH_NAMEDECL(noiseScale, "Noise Amount") = ShConstant1f(0.2f);
  noiseScale.range(0.0f, 1.0f);
  postsh[i++] = ShKernelPost::noisify<ShColor3f>() << (mul<ShAttrib1f>() << htscale << invheight)<< noiseScale; 
  */

  return true;
}

AlgebraShader the_algebra_shader;

