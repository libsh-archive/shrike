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
#include <sstream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

const double DIFFR = 0.05;

class NonPeriodic3D : public Shader {
public:
  NonPeriodic3D(const Globals&);
  ~NonPeriodic3D();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

NonPeriodic3D::NonPeriodic3D(const Globals& globals)
  : Shader("Tiling: 3D Nonperiodic", globals)
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

 
  ShAttrib1f ignore = SH::max(SH::max(p(1)<-0.5, p(1)>1.5), SH::max(p(2)<-0.5, p(2)>1.5));
  ignore = SH::max(ignore, SH::min(my_d>(dmin-1e-7), my_d<(dmax+1e-7)));
  ignore = SH::max(ignore, t > best);

  // Darken edges
  ShAttrib1f diff = SH::min(frac(p(1)+0.5), frac(p(2)+0.5));
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

    opos = m_globals.mvp | ipos; // Compute NDC position
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

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new NonPeriodic3D(globals));
    return list;
  }
}
#else
static StaticLinkedShader<NonPeriodic3D> instance = 
       StaticLinkedShader<NonPeriodic3D>();
#endif
