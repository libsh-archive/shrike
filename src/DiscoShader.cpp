#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class DiscoShader : public Shader {
public:
  DiscoShader();
  ~DiscoShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

DiscoShader::DiscoShader()
  : Shader("Cellnoise Disco Shader")
{
}

DiscoShader::~DiscoShader()
{
}

bool DiscoShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = namedCombine(vsh, keep<ShPoint4f>("posm"));
  vsh = shSwizzle("texcoord", "normal", "halfVec", "lightVec", "posh") << vsh;

  ShAttrib1f SH_DECL(time) = 0.0;
  time.range(0.0f, 4.0f); 

  ShAttrib1f SH_DECL(tileFrequency) = 16.0;
  tileFrequency.range(0.0, 128.0);

  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShProgram discoTiler = SH_BEGIN_PROGRAM() {
    ShInputTexCoord2f SH_DECL(texcoord);
    ShOutputColor3f SH_DECL(kd);
    ShOutputColor3f SH_DECL(ks);

    ShAttrib3f p;
    p(0,1) = (texcoord * tileFrequency); 
    p(2) = 0.0f; // use z for the time parameter

    // use cellnoise to decide when to switch colours on a cell
    ShAttrib1f timestep = cellnoise<1>(p);
    ShAttrib1f timeoffset = time / timestep;
    p(2) = timeoffset;

    // use cellnoise to find colour of a cell and use power to make 
    // colours funkier (and maybe add environment mapping sweetness?)
    kd = 2.0f * cellnoise<3>(p);
    kd(0) = pow(kd(0), ShConstant1f(2.0f));
    kd(1) = pow(kd(1), ShConstant1f(2.0f));
    kd(2) = pow(kd(2), ShConstant1f(2.0f));
    ks = kd;
  } SH_END;
  
  fsh = ShKernelLib::shPhong<ShColor3f>() << shExtract("specExp") << exponent; 
  fsh = fsh << discoTiler;
  return true;
}

void DiscoShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

DiscoShader the_disco_shader;
