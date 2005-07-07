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
