#include <sh/sh.hpp>
#include <sh/shutil.hpp>

using namespace SH;
using namespace ShUtil;

// TODO: these should be in standard library

// reflect vector about surface given by normal
// (do normalize to be safe --- good place to exploit unit flags)
ShVector3f
reflect (ShVector3f v, ShNormal3f n) {
   v = normalize(v);
   n = normalize(n);
   return ShVector3f(2.0f * (n|v) * n - v); 
}

// parabolic mapping from hemisphere to unit square
// note: "shrunk" by 7/8ths to avoid edge effects
// returns a homogenenous texture coordinate...
// ShHTexCoord3f
ShTexCoord3f
parabolic (ShVector3f v) {
  // ShHTexCoord3f u;
  ShTexCoord3f u;
  u(0) = (7.0/8.0)*v(0) + v(2) + 1;
  u(1) = (7.0/8.0)*v(1) + v(2) + 1;
  u(2) = 2.0*(v(2) + 1);
  return u;
}

