#include <sh/sh.hpp>

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

// Reflect vector about surface given by normal.
ShVector3f
reflect (
   ShVector3f v, 
   ShNormal3f n
) {
   v = normalize(v);
   n = normalize(n);
   ShVector3f r = ShVector3f(2.0f * (n|v) * n - v); 
   // TODO: r.unit(true);
   return r;
}

// Refract vector through surface given by normal.
ShVector3f
refract (
   ShVector3f v, 
   ShNormal3f n, 
   ShAttrib1f theta
) {
   v = normalize(v);
   n = normalize(n);
   ShAttrib1f c = (v|n);
   ShAttrib1f k = c*c - 1.0f;
   k = 1.0f + theta*theta*k;
   k = clamp(k, 0.0f, 1.0f);
   ShAttrib1f a = theta;
   ShAttrib1f b = theta*c + sqrt(k);
   ShVector3f r = a*v + b*n;
   // TODO: r.unit(true);
   return r;
}

// Compute fresnel coefficient.
ShAttrib1f
fresnel (
   ShVector3f v, 
   ShNormal3f n, 
   ShAttrib1f theta
) {
   ShVector3f r = reflect(v,n);
   v = normalize(v);   
   n = normalize(n);   
   ShAttrib1f c = pos(n|v);
   ShAttrib1f s = (theta - 1.0f)/(theta + 1.0f);
   s = s*s;
   // return c*1.0f + (1.0f - c)*0.1f;
   return s + (1.0f - s)*pow((1.0f - c),5);
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


