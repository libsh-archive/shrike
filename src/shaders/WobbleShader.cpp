#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class WobbleShader : public Shader {
public:
  WobbleShader();
  ~WobbleShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

WobbleShader::WobbleShader()
  : Shader("Animation: Wobble")
{
}

WobbleShader::~WobbleShader()
{
}

bool WobbleShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("normal") << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShAttrib1f SH_DECL(time);
  time.range(0.0f, 2.0 * M_PI * 10.0f);
  ShAttrib1f SH_DECL(scale) = 1.0f;
  scale.range(0.0f, 10.0f);
  ShAttrib1f SH_DECL(frequency) = 0.5f;
  frequency.range(0.0f, 1.0f);
  ShProgram wobbler = SH_BEGIN_PROGRAM() {
    ShInOutPosition4f pos;
    ShInOutNormal3f normal;

    ShAttrib1f disp = scale * 0.5 * (sin(pos(1) * frequency + time) + 1.0f);

    pos(0,1,2) += normal * disp;
  } SH_END;
  
  vsh = vsh << shExtract("posm") << wobbler;
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 1.0, 1.0);
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::diffuse<ShColor3f>() << diffuse << lightColor;
  return true;
}

WobbleShader the_wobble_shader;
