#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class TexShader : public Shader {
public:
  TexShader();
  ~TexShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

TexShader::TexShader()
  : Shader("Textures: Texcoords")
{
}

TexShader::~TexShader()
{
}

bool TexShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f i;
    ShOutputColor3f o = i(0,1,0);
  } SH_END_PROGRAM;
  return true;
}

void TexShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

TexShader the_tex_shader;

