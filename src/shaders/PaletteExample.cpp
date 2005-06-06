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
#include <iostream>
#include <sstream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class PaletteExample : public Shader {
public:
  PaletteExample();
  ~PaletteExample();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  ShPalette<ShColor3f> colors;

  static PaletteExample instance;
};

PaletteExample::PaletteExample()
  : Shader("Palette Example"),
    colors(5)
{
  colors[0] = ShColor3f(1.0, 0.0, 0.0);
  colors[1] = ShColor3f(0.0, 1.0, 0.0);
  colors[2] = ShColor3f(0.0, 0.0, 1.0);
  colors[3] = ShColor3f(1.0, 1.0, 0.0);
  colors[4] = ShColor3f(0.0, 1.0, 1.0);

  // name the colors
  for (int i = 0; i < 5; i++) {
    std::ostringstream os;
    os << "color" << i;
    colors[i].name(os.str());
  }
}

PaletteExample::~PaletteExample()
{
}

bool PaletteExample::init()
{
  ShAttrib1f SH_DECL(color_selector) = 0.0;
  color_selector.range(0.0, 4.0);
  
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light
    ShOutputColor3f color;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    color = colors[color_selector];
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;
    ShInputColor3f color;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);

    result = pos(normal | light) * color;
  } SH_END;
  return true;
}

PaletteExample PaletteExample::instance = PaletteExample();
