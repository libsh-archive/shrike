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
    AlgebraWrapper(std::string name, int lightidx, int surfidx, int postidx) 
      : Shader(name), lightidx(lightidx), surfidx(surfidx), postidx(postidx) {}

    bool init(); 
    void render();

    ShProgram vertex() { return vsh;}
    ShProgram fragment() { return fsh;}

  private:
    int lightidx, surfidx, postidx;
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
  static const int SURFACE = 6;
  static const int POST = 2;

  static ShProgram lightsh[LIGHT];
  static const char* lightName[LIGHT];

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
ShProgram AlgebraShader::surfsh[AlgebraShader::SURFACE];
ShProgram AlgebraShader::postsh[AlgebraShader::POST];
bool AlgebraShader::doneInit = false;

const char* AlgebraShader::lightName[] = {
  "Point Light",
  "Spot Light",
  "Textured Hemispherical Light"
};

const char* AlgebraShader::surfName[] = {
  "Null Surface",
  "Diffuse Surface",
  "Specular Surface",
  "Phong Surface",
  "Textured Phong Surface",
  "Gooch Surface"
};

const char* AlgebraShader::postName[] = {
  "Null Postprocessor",
  "Halftone Postprocessor"
};

bool AlgebraWrapper::init() {
  AlgebraShader::init_all();
  ShProgram lightsh = AlgebraShader::lightsh[lightidx];
  ShProgram surfsh = AlgebraShader::surfsh[surfidx];
  ShProgram postsh = AlgebraShader::postsh[postidx];

  fsh = namedConnect(lightsh, surfsh);
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
    for(int j = 0; j < SURFACE; ++j) {
      for(int k = 0; k < POST; ++k) {
        std::string name = std::string("Algebra: ") + lightName[i] + " - " + surfName[j] + " - " + postName[k];
        shaders.push_back(new AlgebraWrapper(name, i, j, k)); 
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
  ShColor3f SH_NAMEDECL(cool, "Specular Color") = ShConstant3f(0.4f, 0.4f, 1.0f);
  ShColor3f SH_NAMEDECL(warm, "Specular Color") = ShConstant3f(1.0f, 0.4f, 0.4f);
  ShAttrib1f SH_NAMEDECL(specExp, "Specular Exponent") = ShConstant1f(48.13f);
  specExp.range(0.0f, 256.0f);

// ****************** Make light shaders
  lightsh[0] = ShKernelLight::pointLight<ShColor3f>() << lightColor;
  lightsh[1] = ShKernelLight::spotLight<ShColor3f>() << lightColor << falloff << lightAngle << lightDir;


  ShAttrib1f SH_NAMEDECL(texLightScale, "Mask Scaling Factor") = ShConstant1f(5.0f);
  texLightScale.range(1.0f, 10.0f);
  image.loadPng(SHMEDIA_DIR "/mats/inv_oriental038.png");
  ShTexture2D<ShColor3f> lighttex(image.width(), image.height());
  lighttex.memory(image.memory());
  lightsh[2] = ShKernelLight::texLight2D(lighttex) << texLightScale << lightAngle << lightDir << lightUp;

  //lightsh[1] = ShKernelLight::spotLight(ShColor3f>() << lightColor << falloff << lightAngle << lightDir;
  //lightName[1] = "Spot Light";
  
// ****************** Make surface shaders 
  surfsh[0] = ShKernelSurface::null<ShColor3f>(); 
  surfsh[1] = ShKernelSurface::diffuse<ShColor3f>() << kd;
  surfsh[2] = ShKernelSurface::specular<ShColor3f>() << ks << specExp;
  surfsh[3] = ShKernelSurface::phong<ShColor3f>() << kd << ks << specExp;

  image.loadPng(SHMEDIA_DIR "/textures/rustkd.png");
  ShTexture2D<ShColor3f> difftex(image.width(), image.height());
  difftex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/textures/rustks.png");
  ShTexture2D<ShColor3f> spectex(image.width(), image.height());
  spectex.memory(image.memory());

  surfsh[4] = ShKernelSurface::phong<ShColor3f>() << ( access(difftex) & access(spectex) );
  surfsh[4] = surfsh[4] << shExtract("specExp") << specExp;
  surfsh[5] = ShKernelSurface::gooch<ShColor3f>() << kd << cool << warm; 

// ******************* Make postprocessing shaders
  image.loadPng(SHMEDIA_DIR "/textures/halftone.png");
  ShTexture2D<ShColor3f> halftoneTex(image.width(), image.height());
  halftoneTex.memory(image.memory());


  postsh[0] = keep<ShColor3f>("result"); 

  ShAttrib1f SH_NAMEDECL(htscale, "Scaling Factor") = ShConstant1f(50.0f);
  htscale.range(1.0f, 400.0f);
  postsh[1] = ShKernelPost::halftone<ShColor3f>(halftoneTex) << (mul<ShAttrib1f>() << htscale << invheight);

  return true;
}

AlgebraShader the_algebra_shader;

