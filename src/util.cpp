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

