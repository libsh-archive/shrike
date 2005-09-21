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
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class Checkerboard : public Shader {
public:
  Checkerboard(bool aa);
  ~Checkerboard();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:

  ShProgram vsh, fsh;
  
  bool m_aa;
  static bool m_done_init;
  static ShAttrib1f m_frequency;
  static ShColor3f m_color1, m_color2;
};

Checkerboard::Checkerboard(bool aa)
  : Shader(std::string("Tiling: Checkerboard") + (aa ? ": Antialiased" : ": Aliased")),
    m_aa(aa)
{
  if (!m_done_init) {
    // Initialize static variables
    m_frequency.name("Frequency");
    m_frequency.range(0.0, 10.0);

    SH_NAME(m_color1);
    SH_NAME(m_color2);

    m_done_init = true;
  }
}

Checkerboard::~Checkerboard()
{
}

ShColor3f debugScalar(const ShAttrib1f& scalar)
{
  return scalar * cond(scalar < 0.0,
                       ShConstColor3f(-1.0, 0.0, 0.0),
                       ShConstColor3f( 0.0, 1.0, 0.0));
}

bool Checkerboard::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;


  ShAttrib1f SH_DECL(scale) = 1.0;
  scale.range(1.0, 10.0);
  
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    ShAttrib2f checkpos = frac(tc * m_frequency);

    // This is based on the "Antialised Checkerboard Fragment Shader"
    // from the book "OpenGL Shading Language" by Randi Rost,
    // pp. 350--351, Addison-Wesley.
    // The original is of course written in GLSL.
    if (m_aa) {
      ShColor3f avgcolor = lerp(ShConstAttrib1f(0.5f), m_color1, m_color2);
      ShAttrib2f fw = fwidth(tc);
      ShAttrib2f fuzz = fw * m_frequency * 2.0;
      
      ShAttrib1f fuzzMax = max(fuzz(0), fuzz(1));
      
      ShAttrib2f p = linstep(checkpos, 0.5f + fuzz * 0.5f, fuzz) +
        (1.0  - linstep(checkpos, 0.5f * fuzz, fuzz));
      
      o = lerp(p(0)*p(1) + (1.0 - p(0)) * (1.0 - p(1)), m_color2, m_color1);
      o = lerp(linstep(fuzzMax, ShConstAttrib1f(0.3125), ShConstAttrib1f(0.375)), avgcolor, o);
    } else {
      o = lerp(checkpos(0)*checkpos(1) + (1.0 - checkpos(0))*(1.0 - checkpos(1)) >= 0.5,
               m_color2, m_color1);
    }
  } SH_END_PROGRAM;
  return true;
}

bool Checkerboard::m_done_init = false;
ShAttrib1f Checkerboard::m_frequency = ShAttrib1f(1.0);
ShColor3f Checkerboard::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f Checkerboard::m_color2 = ShColor3f(1.0, 1.0, 1.0);

Checkerboard cb_with_aa = Checkerboard(true);
Checkerboard cb_without_aa = Checkerboard(false);

