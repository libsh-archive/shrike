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

class NonPeriodic : public Shader {
public:
  NonPeriodic();
  ~NonPeriodic();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static NonPeriodic instance;
};

NonPeriodic::NonPeriodic()
  : Shader("Tiling: 1D Nonperiodic")
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
	
  return max(abs(v(0)),abs(v(1)))<0.5;
}

bool NonPeriodic::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
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

NonPeriodic NonPeriodic::instance = NonPeriodic();


