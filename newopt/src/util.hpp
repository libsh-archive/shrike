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

#include "utilimpl.hpp"

/** Compute Lambert's law.   
 * Note that input vectors do not have to be normalized, this
 * function does that.   We depend on the compiler to do domain-specific
 * optimizations to remove redundant calls to normalization as well
 * as removing multiple calls via common subexpression elimination.
 * This returns a scalar which is normally multiplied by the 
 * incoming radiance.
 */
ShAttrib1f 
irradiance (
   ShNormal3f normal, 
   ShVector3f light
);

/** Compute fresnel coefficient.
 * This is the fraction of incident light that should be reflected
 * specularly for dielectrics.   More light tends to be reflected
 * specularly at glancing angles.   This uses Schlick's approximation.
 * Note: some overlap in computation with refract, etc.   However,
 * the compiler is supposed to eliminate common subexpressions, so
 * don't worry about it.   It's better to use these anyways to keep
 * shaders easy to understand and modular.
 *
 * The parameter eta is the relative index of refraction.
 */
ShAttrib1f
fresnel (
   ShVector3f v, 
   ShNormal3f n, 
   ShAttrib1f eta
);


/** Compute the Beckmann's distribution function
 */
ShAttrib1f
beckmann (
  ShVector3f n,
  ShVector3f h,
  ShAttrib1f roughness
);

// TODO: reflect of point about plane should also be supported
// but need ShPlane to be implemented first!

/** Homogeneous parabolic mapping from hemisphere to unit square.
 * This mapping is "shrunk" by 7/8ths to avoid edge effects.
 * Returns a homogenenous texture coordinate, takes a 3D vector
 * (which we normalize; domain-specific optimizations will
 * remove this normalization if it is not needed).
 */
ShTexCoord3f // TODO: really should be ShHTexCoord3f  
parabolic (
   ShVector3f v
);

/** Parabolic mapping from hemisphere to unit square.
 * @see parabolic
 * Like parabolic, but projective normalization is built in.
 */
ShTexCoord2f
parabolic_norm (
   ShVector3f v
);
