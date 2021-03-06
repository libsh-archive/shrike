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
#include "ShTrackball.hpp"
#include <cmath>

#define SH_EPSILON (1.0e-5)

namespace SH {

void ShTrackball::resize(float w, float h)
{
  m_width = w;
  m_height = h;
}

ShMatrix4x4f ShTrackball::rotate(float sx, float sy, float ex, float ey) const
{
  // we get into trouble if we are passed in the same
  // point so ignore it
  if (fabs(sx - ex) < SH_EPSILON && fabs(sy - ey) < SH_EPSILON) {
    return ShMatrix4x4f();
  }

  // get our start point, convert to NDC and the project onto sphere,
  // flip Y due to OpenGL coordinate system...
  ShPoint3f S;
  S(0) = (8.0/3.0)*(sx/m_width - 0.5);
  S(1) = (8.0/3.0)*(-(sy/m_height - 0.5));

  ShAttrib1f z = 1.0 - S(0)*S(0) - S(1)*S(1);
  float zd;
  z.getValues(&zd);
  if (zd < 0.0) {
    S(0) = S(0) / std::sqrt(1.0 - zd);
    S(1) = S(1) / std::sqrt(1.0 - zd);
    S(2) = 0.0;
  } else {
    S(2) = std::sqrt(zd);
  }
  
  // get our end point, convert to NDC and the project onto sphere,
  // flip Y due to OpenGL coordinate system...
  ShPoint3f T;
  T(0) = (8.0/3.0)*(ex/m_width - 0.5);
  T(1) = (8.0/3.0)*(-(ey/m_height - 0.5));

  z = 1.0 - T(0)*T(0) - T(1)*T(1);
  z.getValues(&zd);
  if (zd < 0.0) {
    T(0) = T(0) / std::sqrt(1.0 - zd);
    T(1) = T(1) / std::sqrt(1.0 - zd);
    T(2) = 0.0;
  } else {
    T(2) = std::sqrt(zd);
  }
      
  // get our axis of rotation and the amount of rotation
  ShPoint3f A = normalize(cross(S, T));

  ShAttrib1f theta = 2.0 * sqrt(dot(S-T, S-T));
  float thetad;
  theta.getValues(&thetad);

  if (fabs(thetad) < SH_EPSILON) {
    return ShMatrix4x4f();
  }

  // build our quaternion and produce a matrix
  ShAttrib1f s = std::cos(thetad/2.0);
  ShAttrib1f a = std::sin(thetad/2.0) * A(0);
  ShAttrib1f b = std::sin(thetad/2.0) * A(1);
  ShAttrib1f c = std::sin(thetad/2.0) * A(2);
  ShMatrix4x4f m;
  m[0](0) = (1.0 - 2.0*b*b - 2.0*c*c);
  m[0](1) = 2.0*a*b - 2.0*s*c;
  m[0](2) = 2.0*a*c + 2.0*s*b;
  m[1](0) = 2.0*a*b + 2.0*s*c;
  m[1](1) = 1.0 - 2.0*a*a - 2.0*c*c;
  m[1](2) = 2.0*b*c - 2.0*s*a;
  m[2](0) = 2.0*a*c - 2.0*s*b;
  m[2](1) = 2.0*b*c + 2.0*s*a;
  m[2](2) = 1.0 - 2.0*a*a - 2.0*b*b;
      
  return m;
  
}

}
