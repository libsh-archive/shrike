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
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

DiffuseShader::DiffuseShader()
  : Shader("Diffuse: Algebra")
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
  fsh = ShKernelLib::shDiffuse<ShColor3f>() << color;
  return true;
}

void DiffuseShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

DiffuseShader the_diffuse_shader;

