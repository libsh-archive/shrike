#include <sh/sh.hpp>

using namespace SH;

// TODO: these should be in standard library

// compute irradiance.   Note that calls to normalize
// should (a) exploit unit flag and that (b) we still need
// to set up common subexpression elimination in compiler to
// get rid of redundant calls to "normalize".
// returns a scalar, so really is only "geometric" part
// of computation, should be multiplied by light colour.
ShAttrib1f 
irradiance (ShNormal3f normal, ShVector3f light) {
  normal = normalize(normal);
  light = normalize(light);
  return pos(normal | light);
}


// reflect vector about surface given by normal
// (do normalize to be safe --- good place to exploit unit flags)
ShVector3f
reflect (ShVector3f v, ShNormal3f n) {
   v = normalize(v);
   n = normalize(n);
   return ShVector3f(2.0f * (n|v) * n - v); 
}

// reflect of point about plane should also be supported

// parabolic mapping from hemisphere to unit square
// note: "shrunk" by 7/8ths to avoid edge effects
// returns a homogenenous texture coordinate...
// expects a normalized vector.   We do normalization
// here, under assumption that unit flags in vectors will
// avoid repeated normalization.
// ShHTexCoord3f
ShTexCoord3f
parabolic (ShVector3f v) {
  // ShHTexCoord3f u;
  ShTexCoord3f u;
  v = normalize(v);
  u(0) = (7.0/8.0)*v(0) + v(2) + 1;
  u(1) = (7.0/8.0)*v(1) + v(2) + 1;
  u(2) = 2.0*(v(2) + 1);
  return u;
}

// as above, but with projective normalization built in
ShTexCoord2f
parabolic_norm (ShVector3f v) {
  ShTexCoord3f hu = parabolic(v);
  ShAttrib1f r = 1.0f/hu(2);
  ShTexCoord2f u;
  u(0) = r * hu(0);
  u(1) = r * hu(1);
  return u;
}


