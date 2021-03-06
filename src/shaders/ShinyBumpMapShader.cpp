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

class ShinyBumpMapShader : public Shader {
public:
  ShProgram vsh, fsh;

  ShinyBumpMapShader(const Globals&);
  ~ShinyBumpMapShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

ShinyBumpMapShader::ShinyBumpMapShader(const Globals& globals)
  : Shader("Bump and Frame Mapping: Shiny Bump Mapping", globals)
{
}

ShinyBumpMapShader::~ShinyBumpMapShader()
{
}

bool ShinyBumpMapShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  load_PNG(test_image, normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png"));

  ShTableCube<ShColor4fub> cubemap(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      load_PNG(image, normalize_path(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png"));
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc;    // texture coordinates
    ShOutputNormal3f onorm; // view-space normal
    ShOutputVector3f otan; // view-space tangent
    ShOutputVector3f viewv;  // view vector

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal
    otan = m_globals.mv | itan; // Compute view-space tangent
    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    viewv = -ShVector3f(posv); // Compute view vector
  } SH_END;

  ShImage image;
  load_PNG(image, normalize_path(SHMEDIA_DIR "/bumpmaps/bumps_normals.png"));
  ShTable2D<ShVector3fub> bump(image.width(),image.height());
  bump.memory(image.memory());

  ShAttrib3f SH_DECL(scale) = ShAttrib3f(2.0,2.0,1.0);
  scale.range(0.0f,10.0f);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f n;
    ShInputVector3f t;
    ShInputVector3f v;

    ShOutputColor3f result;
    
    ShVector3f s = cross(t,n);
    t = normalize(t);
    s = normalize(s);
    n = normalize(n);
    ShVector3f b = bump(u) - ShAttrib3f(0.5,0.5,0.0);
    b *= scale;
    ShVector3f bn = t * b(0) + s * b(1) + n * b(2);
    ShVector3f r = reflect(v,bn); // Compute reflection vector

    // actually do reflection lookup in model space
    r = m_globals.mv_inverse | r;

    result = cubemap(r)(0,1,2); 
  } SH_END;

  return true;
}


#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new ShinyBumpMapShader(globals));
    return list;
  }
}
#else
static StaticLinkedShader<ShinyBumpMapShader> instance = 
       StaticLinkedShader<ShinyBumpMapShader>();
#endif
