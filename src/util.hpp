
#include "utilimpl.cpp"

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

/** Reflect vector about surface given by normal.
 * Note that input vectors do not have to be normalized, this
 * function does that.   We depend on the compiler to do domain-specific
 * optimizations to remove redundant calls to normalization as well
 * as removing multiple calls via common subexpression elimination.
 * Returns the reflected vector (which will be unit length; we mark this
 * fact so the domain-specific optimizer can do the right thing later).
 */
ShVector3f
reflect (
   ShVector3f v, 
   ShNormal3f n
);

/** Refract vector through surface given by normal.
 * Note that input vectors do not have to be normalized, this
 * function does that.   We depend on the compiler to do domain-specific
 * optimizations to remove redundant calls to normalization as well
 * as removing multiple calls via common subexpression elimination.
 * Returns the refracted vector (which will be unit length; we mark this
 * fact so the domain-specific optimizer can do the right thing later).
 */
ShVector3f
refract (
   ShVector3f v,      ///< incident vector
   ShNormal3f n,      ///< surface normal
   ShAttrib1f theta   ///< relative index of refraction
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
 * The parameter theta is the relative index of refraction.
 */
ShAttrib1f
fresnel (
   ShVector3f v, 
   ShNormal3f n, 
   ShAttrib1f theta
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

