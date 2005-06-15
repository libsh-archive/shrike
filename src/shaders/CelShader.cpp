// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
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

  ShWrapRepeat< ShTexture2D<ShColor3fub> > paper(256, 256);
  ShImage paperImg;
  paperImg.loadPng(normalize_path(SHMEDIA_DIR "/textures/paper.png"));
  paper.memory(paperImg.memory());

  ShWrapRepeat< ShTexture2D<ShColor3fub> > hatching(256, 256);
  ShImage hatchingImg;
  hatchingImg.loadPng(normalize_path(SHMEDIA_DIR "/textures/hatching.png"));
  hatching.memory(hatchingImg.memory());
  
  ShColor3f SH_DECL(color1) = ShColor3f(0.0, 0.0, 0.0);
  ShColor3f SH_DECL(color2) = ShColor3f(0.4, 0.45, 0.5);
  ShColor3f SH_DECL(color3) = ShColor3f(0.9, 0.88, 0.82);
  ShColor1f SH_DECL(cutoff1) = 0.15f;
  ShColor1f SH_DECL(cutoff2) = 0.35f;
  ShColor3f SH_DECL(spec) = ShColor3f(-1.0, -1.0, -1.0);
  spec.range(-1.0, 0.0);
  ShColor1f SH_DECL(diffuse) = ShColor1f(1.0);
  ShAttrib1f SH_DECL(exp) = ShAttrib1f(30.0);
  exp.range(5.0, 500.0);
  ShColor1f SH_DECL(hatch) = ShColor1f(0.0);
  ShColor1f SH_DECL(hatchcut) = ShColor1f(0.1);
  ShColor1f SH_DECL(hatchblend) = ShColor1f(0.4);
  ShAttrib1f SH_DECL(hatchscale) = ShAttrib1f(1.0);
  hatchscale.range(0.0f, 10.0f);

  ShProgram lookup = SH_BEGIN_PROGRAM() {
    ShInputColor1f intensity;
    ShOutputColor3f out;
    out = cond(intensity > cutoff2, color3, cond(intensity > cutoff1, color2, color1));
  } SH_END_PROGRAM;

  lookup = lookup << ShKernelLib::shDiffuse<ShColor1f>() << ShConstant1f(1.0);

  ShProgram postex = SH_BEGIN_PROGRAM() {
    ShInputPosition4f ipos;
    ShOutputTexCoord2f opos = ipos(0, 1) * ShAttrib2f(1.0f/400.0f, -1.0f/400.f);
  } SH_END_PROGRAM;

  ShProgram hatcher = SH_BEGIN_PROGRAM() {
    ShInputColor1f diffuse;
    ShInputTexCoord2f tc;
    ShOutputColor3f color;

    ShColor1f blend = cond(diffuse <= hatchcut,
                           ShColor1f(1.0f),
                           cond(diffuse <= hatchblend,
                                1.0f - (diffuse - hatchcut)/(hatchblend - hatchcut),
                                ShColor1f(0.0f)));
    color = lerp(blend, hatching(tc * hatchscale), ShColor3f(1.0, 1.0, 1.0));
  } SH_END_PROGRAM;
  
  fsh = mul<ShColor3f>() << add<ShColor3f>() << mul<ShColor3f>()
                         << (lookup
                             & hatcher << ShKernelLib::shDiffuse<ShColor1f>() << diffuse
                             & ShKernelLib::shSpecular<ShColor3f>() << spec << exp
                             & access(paper) << postex)
                         << (shRange
                             ("normal")("viewVec")("posh")
                             ("normal")("lightVec")("posh")
                             ("texcoord")
                             ("normal")("halfVec")("lightVec")("posh")
                             ("posh")
                             << ShKernelLib::outputPass(vsh));
  return true;
}


CelShader the_cel_shader;

