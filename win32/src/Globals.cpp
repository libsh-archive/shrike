#include "Globals.hpp"

SH::ShPoint3f Globals::lightPos = SH::ShPoint3f(0.0, 10.0, 10.0);
SH::ShVector3f Globals::lightDirW = SH::ShVector3f(0.0, 1.0, 1.0);
SH::ShAttrib1f Globals::lightLenW = 5.0;
SH::ShMatrix4x4f Globals::mv = SH::ShMatrix4x4f();
SH::ShMatrix4x4f Globals::mv_inverse = SH::ShMatrix4x4f();
SH::ShMatrix4x4f Globals::mvp = SH::ShMatrix4x4f();
