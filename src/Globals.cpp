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
#include "Globals.hpp"

Globals &GetGlobals()
{
  static Globals globals;
  return globals;
}

/*
SH::ShPoint3f Globals::lightPos = SH::ShPoint3f(0.0, 10.0, 10.0);
SH::ShVector3f Globals::lightDirW = SH::ShVector3f(0.0, 1.0, 1.0);
SH::ShAttrib1f Globals::lightLenW = 5.0;
SH::ShMatrix4x4f Globals::mv = SH::ShMatrix4x4f();
SH::ShMatrix4x4f Globals::mv_inverse = SH::ShMatrix4x4f();
SH::ShMatrix4x4f Globals::mvp = SH::ShMatrix4x4f();
*/
