// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
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
#ifndef HDR3DTEX_H
#define HDR3DTEX_H

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class CubeMap : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef CubeMap<typename T::rectangular_type> rectangular_type;
	
	CubeMap() : parent_type()
	{}

	CubeMap(int width) : parent_type(width)
	{}

	CubeMap(int width, int height) : parent_type(width, height)
	{}

	CubeMap(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	return_type operator[](const ShAttrib3f u) const {
		const T *bt = this;			
		return (*bt)[u]; // does not exist ?
	}
			
	return_type operator()(const ShAttrib3f u) const {
		const T *bt = this;
		ShAttrib3f v = normalize(u);
		ShAttrib3f absv = abs(v);
		ShAttrib1f maxcoord = SH::max(SH::max(absv(0), absv(1)), absv(2)); // find the max coordinates
		ShAttrib3f textcoord = -v / maxcoord;
		// find which face is reached
		ShAttrib1f Xface = textcoord(0) * (absv(0) > SH::max(absv(1), absv(2)));
		ShAttrib1f Yface = textcoord(1) * (absv(1) > SH::max(absv(0), absv(2)));
		ShAttrib1f Zface = textcoord(2) * (absv(2) > SH::max(absv(0), absv(1)));
		ShAttrib2f tc;
		// find the coordinates cooresponding to the face reached
		tc = cond(abs(Xface), ShAttrib2f(textcoord(2),textcoord(1)) , cond(abs(Yface), ShAttrib2f(textcoord(0), textcoord(2)), ShAttrib2f(textcoord(0), textcoord(1))));
		// translate and scale to get the right size
		tc += ShAttrib2f(1.0,1.0);
		tc *= ShAttrib2f(0.25,0.1666666);
		//translate texture coordinates to reach the right part of the texture
		tc(0) = cond(Xface+Yface+Zface, tc(0), tc(0)+0.5);
		tc(1) = cond(abs(Xface), tc(1)+0.333333, tc(1));
		tc(1) = cond(abs(Yface), tc(1)+0.666667, tc(1));
		return (*bt)(tc);
	}
};

#endif
