// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
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
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class SimpleDiffuse : public Shader {
public:
  SimpleDiffuse();
  ~SimpleDiffuse();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SimpleDiffuse instance;
};

SimpleDiffuse::SimpleDiffuse()
  : Shader("Basic Lighting Models: Diffuse: Simple")
{
}

SimpleDiffuse::~SimpleDiffuse()
{
}

bool SimpleDiffuse::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(.2, 0.5, 0.9);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);

    result = pos(normal | light) * color;
  } SH_END;
  return true;
}

SimpleDiffuse SimpleDiffuse::instance = SimpleDiffuse();


