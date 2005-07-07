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
#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <sh/sh.hpp>

// TODO: Improve on this. 
struct Globals {
  static SH::ShMatrix4x4f mv;
  static SH::ShMatrix4x4f mv_inverse;
  static SH::ShMatrix4x4f mvp;
  static SH::ShPoint3f lightPos; // in view space
  static SH::ShVector3f lightDirW; // in world space
  static SH::ShAttrib1f lightLenW; // in world space
};

#endif
