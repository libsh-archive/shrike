#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <sh/sh.hpp>

// TODO: Improve on this. 
struct Globals {
  static SH::ShMatrix4x4f mv;
  static SH::ShMatrix4x4f mv_inverse;
  static SH::ShMatrix4x4f mvp;
  static SH::ShPoint3f lightPos; // in view space
  static SH::ShPoint3f lightPosW; // in world space
};

#endif
