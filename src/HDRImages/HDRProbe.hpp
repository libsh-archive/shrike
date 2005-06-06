// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#ifndef HDRPROBE_H
#define HDRPROBE_H

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class Probe : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
  typedef Probe<typename T::rectangular_type> rectangular_type;
  
  Probe() : parent_type() {}
  Probe(int width) : parent_type(width) {}
  Probe(int width, int height) : parent_type(width, height) {}
  Probe(int width, int height, int depth) : parent_type(width, height, depth) {}
	
  return_type operator[](const ShAttrib3f u) const 
  {
    const T *bt = this;			
    ShAttrib1f r = acos(u(2))/(M_PI*sqrt(u(0)*u(0)+u(1)*u(1)));
    ShAttrib2f tc = r*ShAttrib2f(u(0),u(1));
    tc += ShAttrib2f(1.0,1.0);
    tc /= 2.0;
    tc *= size();
    return (*bt)[tc];
  }
			
  return_type operator()(const ShAttrib3f u) const 
  {
    const T *bt = this;
    ShAttrib1f r = acos(u(2))/(M_PI*sqrt(u(0)*u(0)+u(1)*u(1)));
    ShAttrib2f tc = r*ShAttrib2f(u(0),u(1));
    tc += ShAttrib2f(1.0,1.0);
    tc /= 2.0;
    return (*bt)(tc);
  }
};

#endif
