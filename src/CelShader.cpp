#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class CelShader : public Shader {
public:
  CelShader();
  ~CelShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;
};

CelShader::CelShader()
  : Shader("Cel (toon) shading")
{
}

CelShader::~CelShader()
{
}

bool CelShader::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "halfVec", "lightVec", "posh") << vsh;
  
  ShColor3f SH_DECL(color1) = ShColor3f(0.0, 0.0, 0.0);
  ShColor3f SH_DECL(color2) = ShColor3f(0.5, 0.5, 0.5);
  ShColor3f SH_DECL(color3) = ShColor3f(.7, 0.0, 0.0);
  ShColor1f SH_DECL(cutoff1) = 0.01f;
  ShColor1f SH_DECL(cutoff2) = 0.15f;
  ShColor3f SH_DECL(spec) = ShColor3f(1.0, 1.0, 1.0);
  ShAttrib1f SH_DECL(exp) = ShAttrib1f(35.0);
  exp.range(5.0, 500.0);

  ShProgram lookup = SH_BEGIN_PROGRAM() {
    ShInputColor1f intensity;
    ShOutputColor3f out;
    out = cond(intensity > cutoff2, color3, cond(intensity > cutoff1, color2, color1));
  } SH_END_PROGRAM;

  lookup = lookup << ShKernelLib::shDiffuse<ShColor1f>() << ShConstant1f(1.0);

  fsh = add<ShColor3f>() << (lookup & (ShKernelLib::shSpecular<ShColor3f>() << spec << exp))
                         << (shSwizzle("normal", "lightVec", "posh",
                                       "normal", "halfVec", "lightVec", "posh")
                             << ShKernelLib::outputPass(vsh));
  return true;
}

void CelShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  shBindShader(vsh);
  shBindShader(fsh);
}

CelShader the_cel_shader;

