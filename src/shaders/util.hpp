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

