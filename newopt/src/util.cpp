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

#ifndef M_E
#define M_E 2.7182818284590452354
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace SH;
#include "util.hpp"

// TODO: these should be in standard library

// Compute Lambert's law.   
ShAttrib1f 
irradiance (
  ShNormal3f normal, 
  ShVector3f light
) {
  normal = normalize(normal);
  light = normalize(light);
  return pos(normal | light);
}

// Compute fresnel coefficient.
ShAttrib1f
fresnel (
   ShVector3f v, 
   ShNormal3f n, 
   ShAttrib1f eta
) {
   v = normalize(v);   
   n = normalize(n);   
   ShAttrib1f c = pos(n|v);
   ShAttrib1f s = (eta - 1.0f)/(eta + 1.0f);
   s = s*s;
   // return c*1.0f + (1.0f - c)*0.1f;
   return s + (1.0f - s)*pow((1.0f - c),5);
}

// Compute the Beckmann's distribution factor
ShAttrib1f
beckmann (
   ShVector3f n,
   ShVector3f h,
   ShAttrib1f roughness
) {
   ShAttrib1f normalDotHalf = (n | h);
   ShAttrib1f normalDotHalf2 = normalDotHalf * normalDotHalf;
   ShAttrib1f roughness2 = roughness * roughness;
   ShAttrib1f exponent = -(1 - normalDotHalf2) / 
	   (normalDotHalf2 * roughness2); // Compute the exponent value
   return pow(M_E, exponent) /
	   (roughness2 * normalDotHalf2*normalDotHalf2); // Compute the distribution function
}

	

   
// TODO: reflect of point about plane should also be supported
// but need ShPlane to be implemented first!

// Homogeneous parabolic mapping from hemisphere to unit square.
ShTexCoord3f // TODO: really should be ShHTexCoord3f  
parabolic (
  ShVector3f v
) {
  // ShHTexCoord3f u;
  ShTexCoord3f u;
  v = normalize(v);
  u(0) = (7.0/8.0)*v(0) + v(2) + 1;
  u(1) = (7.0/8.0)*v(1) + v(2) + 1;
  u(2) = 2.0*(v(2) + 1);
  return u;
}

// Parabolic mapping from hemisphere to unit square.
ShTexCoord2f
parabolic_norm (
  ShVector3f v
) {
  ShTexCoord3f hu = parabolic(v);
  ShAttrib1f r = 1.0f/hu(2);
  ShTexCoord2f u;
  u(0) = r * hu(0);
  u(1) = r * hu(1);
  return u;
}

