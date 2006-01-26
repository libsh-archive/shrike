// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////
#include <sh/sh.hpp>
#include <shutil/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class BumpMapShader : public Shader {
public:
  BumpMapShader(const Globals&);
  ~BumpMapShader();
    
  bool init();
    
  ShProgram vertex() { return vsh; }
  ShProgram fragment() { return fsh; }

public:
  ShProgram vsh;
  ShProgram fsh;
};

BumpMapShader::BumpMapShader(const Globals& globals) :
  Shader("Bump and Frame Mapping: Diffuse Bump Mapping", globals)
{
}

BumpMapShader::~BumpMapShader()
{
}

bool BumpMapShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  // stock vertex shader, feed in global light, swizzle
  // out texcoord for bumpmap, normal/tanget to build
  // local frame and lightVec for diffuse lighting
  vsh = ShKernelLib::shVsh(m_globals.mv, m_globals.mvp, 1, 1);
  vsh = vsh << shExtract("lightPos") << m_globals.lightPos;
  vsh = (shSwizzle("texcoord", "normal", "tangent", "lightVec", "posh") << vsh);

  ShImage image;
  load_PNG(image, normalize_path(SHMEDIA_DIR "/bumpmaps/bumps_normals.png"));
  ShTable2D<ShVector3fub> bump(image.width(),image.height());
  bump.memory(image.memory());

  ShAttrib1f SH_DECL(scale) = ShAttrib1f(1.0);
  scale.range(0.0f, 10.0f);
  ShColor3f SH_DECL(diffuse) = ShColor3f(0.5, 0.5, 0.5);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f iposh;
    ShInputTexCoord2f itex;
    ShInputNormal3f inrm;
    ShInputVector3f itan;
    ShInputVector3f ilv;

    ShOutputColor3f result;
    
    // generate bi-tangent and normalize local frame
    ShVector3f btan = cross(itan, inrm);
    itan = normalize(itan);
    btan = normalize(btan);
    inrm = normalize(inrm);

    // lookup perturbed normal, scale [0,1] -> [-1, 1],
    // apply scale factor, renormalize and tranformat
    ShVector3f nrm = 2.0f*bump(itex) - 1.0f;
    nrm(2) = scale*nrm(2);
    nrm = normalize(nrm);
    nrm = itan*nrm(0) + btan*nrm(1) + inrm*nrm(2);

    // perform standard diffuse lighting
    result = diffuse*dot(nrm, ilv);
  } SH_END;

  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new BumpMapShader(globals));
    return list;
  }
}
#else
static StaticLinkedShader<BumpMapShader> instance = 
       StaticLinkedShader<BumpMapShader>();
#endif
