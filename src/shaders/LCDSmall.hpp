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
#ifndef LCDSmall_HPP
#define LCDSmall_HPP
#include <sh/sh.hpp>
#include "Shader.hpp"

class LCDSmall : public Shader {
public:
  LCDSmall();
  ~LCDSmall();

  bool init();

  SH::ShProgram vertex() { return vsh;}
  SH::ShProgram fragment() { return fsh;}

  SH::ShProgram vsh, fsh;

  static LCDSmall instance;
};

SH::ShAttrib1f lcdSmall(const SH::ShTexCoord2f& tc, SH::ShAttrib1f number,
			int intDigits = 3, int fracDigits = 1, bool showgrid = false, 
			bool handleneg = true, float w = 0.2, float h = 0.5, float t = 0.02);

#endif
