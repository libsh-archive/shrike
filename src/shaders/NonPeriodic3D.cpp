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
#include <sstream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

const double DIFFR = 0.05;

class NonPeriodic3D : public Shader {
public:
  NonPeriodic3D();
  ~NonPeriodic3D();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static NonPeriodic3D instance;
};

NonPeriodic3D::NonPeriodic3D()
  : Shader("Tiling: 3D Nonperiodic")
{
}

NonPeriodic3D::~NonPeriodic3D()
{
}

ShColor3f color[3] = {ShColor3f(1.0f, 0.2f, 0.2f), ShColor3f(0.2f, 1.0f, 0.2f), ShColor3f(0.2f, 0.2f, 1.0f)};
ShAttrib1f edgewidth(0.05);
 
ShPoint3f modPoint(const ShPoint3f& p)
{
  return frac(p + ShConstAttrib3f(0.5, 0.5, 0.5)) - ShConstAttrib3f(0.5,0.5,0.5);  
}

void testPlane(const ShVector3f& body, const ShAttrib4f& plane, const ShPoint3f& pt, ShColor3f col,
               ShColor3f& C, ShAttrib1f& best)
{
  ShPoint4f pth;
  pth(0,1,2) = pt;
  pth(3) = 1.0;
  ShAttrib1f t = -(plane | pth) / (body | plane(0,1,2));
  
  ShPoint3f p = pt + t*body;

  ShAttrib1f xmin = floor(p(1)+0.5)-0.5;
  ShAttrib1f ymin = floor(p(2)+0.5)-0.5;
  
  ShAttrib1f dmax = body | ShVector3f(plane(3), -xmin, -ymin);
  ShAttrib1f dmin = body | ShVector3f(plane(3), -xmin-1.0, -ymin-1.0);
  ShAttrib1f my_d = -(body | pt);

 
  ShAttrib1f ignore = max(max(p(1)<-0.5, p(1)>1.5), max(p(2)<-0.5, p(2)>1.5));
  ignore = max(ignore, min(my_d>(dmin-1e-7), my_d<(dmax+1e-7)));
  ignore = max(ignore, t > best);

  // Darken edges
  ShAttrib1f diff = min(frac(p(1)+0.5), frac(p(2)+0.5));
  col = min(diff/edgewidth, 1.0) * col;
  
  C = cond(ignore, C, col);
  best = cond(ignore, best, t);
}

ShColor3f testCube(ShVector3f body, ShPoint3f p)
{
  ShColor3f result;
  ShAttrib1f best = 100000.0;

  for (int i = 0; i < 3; i++) {
    testPlane(body, ShConstAttrib4f(1.0, 0.0, 0.0, -0.5), p, color[i], result, best);
    testPlane(body, ShConstAttrib4f(1.0, 0.0, 0.0, -1.5), p, color[i], result, best);
    body = body(1,2,0);
    p = p(1,2,0);
  }
  
  return result;
}

bool NonPeriodic3D::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShAttrib2f SH_DECL(scale) = ShAttrib2f(10.0, 10.0);
  scale.range(1.0, 100.0);

  ShVector3f SH_DECL(body) = ShVector3f(M_PI/2.0, M_E/2.0, 1.0);
  body.name("plane orientation");
  body.range(0.0, 20.0);
  edgewidth.name("Edge width");
  edgewidth.range(0.0, 0.5);

  for (int i = 0; i < 3; i++) {
    std::ostringstream os;
    os << "Colour " << i;
    color[i].name(os.str());
  }

  // This is a 3D nonperiodic tiling from Craig Kaplan.
  // It should be possible to extend this to make Penrose tiles.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords
    
    ShOutputColor3f result;

    tc = scale*tc;
  
    ShVector3f nbody = normalize(body);
    ShVector3f v1;
    v1[0] = 1.0/nbody(0);
    v1[1] = -1.0/nbody(1);
    v1[2] = 0.0;
    v1 = normalize(v1);

    ShVector3f v2 = cross(nbody,v1);
    v2 = normalize(v2);
    
    ShPoint3f p = tc(0)*v1 + tc(1)*v2;
    p = modPoint(p);
    
    result = testCube(nbody, p);
    
  } SH_END;
  return true;
}

NonPeriodic3D NonPeriodic3D::instance = NonPeriodic3D();






