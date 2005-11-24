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

class Pants : public Shader {
public:
  Pants(const Globals&);
  ~Pants();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

Pants::Pants(const Globals& globals)
  : Shader("Noise: Dad's Pants", globals)
{
}

Pants::~Pants()
{
}

bool Pants::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = m_globals.mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(.2, 0.5, 0.9);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0, 1.0);
  scale.range(1.0, 1000.0);

  ShAttrib1f SH_DECL(power) = 12.0;
  power.range(0.0, 32.0);

  // This simple but cool Moire pattern function is due to Craig Kaplan.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords

    ShOutputColor3f result;

    tc *= scale;
    ShAttrib1f d = dot(tc,tc);
    ShAttrib1f r = frac(d * pow(2.0, power));
    
    result = r * color;
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new Pants(globals));
    return list;
  }
}
#else
static StaticLinkedShader<Pants> instance = 
       StaticLinkedShader<Pants>();
#endif
