#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <list>
#include "Shader.hpp"
#include "Globals.hpp"
#include "ShrikeCanvas.hpp"

using namespace SH;
using namespace ShUtil;

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
  static const int SURFACE = 4;
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
  "Textured Hemispherical Light"
};

const char*  AlgebraShader::surfmapName[] = {
  "Identity Mapping",
  "Bump Mapping"
};

const char* AlgebraShader::surfName[] = {
  "Null Surface",
  "Diffuse Surface",
//  "Specular Surface",
//  "Phong Surface",
  "Textured Phong Surface",
  "Gooch Surface"
};

const char* AlgebraShader::postName[] = {
  "Null Postprocessor",
  "Halftone Postprocessor",
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

  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
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
  surfsh[i++] = ShKernelSurface::gooch<ShColor3f>() << kd << cool << warm; 

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

