#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class DiffuseShader : public Shader {
public:
  DiffuseShader();
  ~DiffuseShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static DiffuseShader instance;
};

DiffuseShader::DiffuseShader()
  : Shader("Basic Lighting Models: Diffuse: Algebra")
{
}

DiffuseShader::~DiffuseShader()
{
}

bool DiffuseShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShColor3f SH_DECL(color) = ShColor3f(.5, 0.9, 0.2);
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::diffuse<ShColor3f>() << color << lightColor;
  return true;
}

DiffuseShader DiffuseShader::instance = DiffuseShader();


