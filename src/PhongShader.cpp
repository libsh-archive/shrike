#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class PhongShader : public Shader {
public:
  PhongShader();
  ~PhongShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

PhongShader::PhongShader()
  : Shader("Phong: Algebra")
{
}

PhongShader::~PhongShader()
{
}

bool PhongShader::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal","halfVec", "lightVec", "posh", "texcoord") << vsh;

  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShImage image;
  image.loadPng(SHMEDIA_DIR "/textures/rustkd.png");
  ShTexture2D<ShColor3f> difftex(image.width(), image.height());
  difftex.memory(image.memory());
  image.loadPng(SHMEDIA_DIR "/textures/rustks.png");
  ShTexture2D<ShColor3f> spectex(image.width(), image.height());
  spectex.memory(image.memory());
  
  fsh = ShKernelLib::shPhong<ShColor3f>()
    << (access(difftex) & access(spectex) & (keep<ShAttrib1f>() << exponent))
    << (shDup() << (shExtract("texcoord")
                    << ShKernelLib::outputPass(vsh)));
  return true;
}

PhongShader the_phong_shader;

