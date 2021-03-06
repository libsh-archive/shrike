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

class NonPeriodic : public Shader {
public:
  NonPeriodic(const Globals&);
  ~NonPeriodic();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

NonPeriodic::NonPeriodic(const Globals& globals)
  : Shader("Tiling: 1D Nonperiodic", globals)
{
}

NonPeriodic::~NonPeriodic()
{
}

static const double m = 0.5*(sqrt( 5.0 ) + 1.0);
static const double m2 = 0.5*(sqrt( 5.0) - 1.0);

ShAttrib1f isRed(ShAttrib1f x, ShMatrix3x3f M)
{
  ShAttrib1f mx = frac( x );
  ShAttrib1f my = frac( m2*x );

  ShVector3f v = ShVector3f(mx,my,ShAttrib1f(1.0));
  v = M | v;
	
  return SH::max(abs(v(0)),abs(v(1)))<0.5;
}

bool NonPeriodic::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = m_globals.mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShColor3f SH_DECL(color1) = ShColor3f(.2, 0.5, 0.9);
  ShColor3f SH_DECL(color2) = ShColor3f(.9, 0.5, 0.2);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(20.0, 20.0);
  scale.range(1.0, 1000.0);

  ShAttrib1f SH_DECL(second) = 0.0;
  second.range(0.0, 1.0);

  ShMatrix3x3f M = ShMatrix3x3f();
  M[0][1]=-m;
  M[1][0]=m;
  M[0][2]=0.5*(m-1.0);
  M[1][2]=-0.5*(m+1.0);
	
  // This is a 1D nonperiodic tiling from Craig Kaplan.
  // It should be possible to extend this to make Penrose tiles.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords

    ShOutputColor3f result;

    tc *= scale;
    result = cond(isRed(tc(0),M) || (second && isRed(tc(1),M)),
		  color1, color2);
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new NonPeriodic(globals));
    return list;
  }
}
#else
static StaticLinkedShader<NonPeriodic> instance = 
       StaticLinkedShader<NonPeriodic>();
#endif
