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

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static TexShader instance;
};

TexShader::TexShader()
  : Shader("Debugging: Texture Coordinates")
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


TexShader TexShader::instance = TexShader();

