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
#ifndef HDRMIPMAPPING_HPP
#define HDRMIPMAPPING_HPP

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

/** Integrate the mip-map function
  *
  * the updateMipMap() function creates a new object
  * to save the data on the mipmap levels
  *
  * the levels are accessed in function of the derivatives of the texture coordinates
  *
  */
template<typename T>
class MipMap : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef MipMap<typename T::rectangular_type> rectangular_type;

	MipMap() : parent_type() {}

	MipMap(int width) : parent_type(width) {}

	MipMap(int width, int height) : parent_type(width, height) {}

	MipMap(int width, int height, int depth) : parent_type(width, height, depth) {}

	// create a new texture to save the original texture and all the mipmap levels
	void updateMipMap() {
		int width = m_node->width();
		int halfwidth = width/2;
		int height = m_node->height();
		int stride = return_type::typesize;
    ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		m_realwidth = width + halfwidth + (width % 2); // the width of the new texture
    ShHostMemoryPtr newmem = new ShHostMemory(m_realwidth * height * stride * sizeof(float)); // create the new object
    m_realtex.memory(newmem);
    m_realtex.size(m_realwidth, m_node->height());
		
		float* olddata = (float*)cursto->data();
		float* newdata = (float*)newmem->hostStorage()->data();

    // copy from old data to new data
   	for (int y = 0; y < height; y++) {
   	 	for (int x = 0; x < width; x++) {
				for (int e = 0; e < stride; e++) {
  				newdata[(y*m_realwidth + x)*stride + e] =	olddata[(y*width + x)*stride + e];
        }
      }
		}
		generateMipMap(newdata, 0, 0, width, height, width, height/2); // add mipmap levels by recursion
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
		ShAttrib2f u = fwidth(tc); // derivatives of the texture coordinates
		ShAttrib1f level = pos(log2(SH::max(u))); // compute the mip-map level
		u = tc;
		if (m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT) { // wraping
			u = size()*frac(u/size());
		}
		ShAttrib1f scale = pow(0.5,floor(level)); // compute the reduction factor
		ShAttrib1f transition = frac(level); // the transition between 2 levels
		u *= scale; // reduce the size
		ShAttrib2f u2 = 0.5*u + size()*ShAttrib2f(ShAttrib1f(1.0),0.5*scale); // coordinates for the next level
		u = cond(level>=1.0, u+size()*ShAttrib2f(ShAttrib1f(1.0),scale), u);
		return (1.0-transition)*m_realtex[u] + transition*m_realtex[u2]; // do linear interpolation between the levels
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		ShAttrib2f u = fwidth(tc)*size();// derivatives of the texture coordinates
		ShAttrib1f level = pos(log2(SH::max(u))); // compute the mip-map level
		u = tc;
		if (m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT) {
			u = frac(u);
		}
		ShAttrib1f twothird(0.666666); // scale to the size of the original texture
		u(0) *= twothird; // scale to the size of the original data
		ShAttrib1f scale = pow(0.5,floor(level)); // compute the reduction factor
		ShAttrib1f transition = frac(level);
		u *= scale; // reduce the size
		ShAttrib2f u2 = 0.5*u + ShAttrib2f(twothird,0.5*scale); // coordinates for the next level
		u = cond(level>=1.0, u+ShAttrib2f(twothird,scale), u); // translate to the right position
		return (1.0-transition)*m_realtex(u) + transition*m_realtex(u2); // linear interpolation between the levels
	}

private:
  /** Add the mip-map levels by reducing the data size by a factor 2
    * an calling itself on the result
    */
	void generateMipMap(float* texdata, int wstart, int hstart, int wend, int hend, int wnew, int hnew) {
		int stride = return_type::typesize;
		int width = (wend-wstart)/2; // the size of the mip-map level
		int height = (hend-hstart)/2;
		// do a linear interpolation to resample the image
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				for(int e=0 ; e<stride ; e++) {
					int maxx = 2*x+1 < 2*width ? 2*x+1 : 2*width-1; // compute the limits of the coordinates
					int maxy = 2*y+1 < 2*height ? 2*y+1 : 2*height-1;
					texdata[((y+hnew)*m_realwidth + x+wnew)*stride + e] =	texdata[((2*y+hstart)*m_realwidth + 2*x+wstart)*stride + e] +
																																texdata[((2*y+hstart)*m_realwidth + maxx+wstart)*stride +e] +
																																texdata[((maxy+hstart)*m_realwidth + 2*x+wstart)*stride +e] +
																																texdata[((maxy+hstart)*m_realwidth + maxx+wstart)*stride +e];
					texdata[((y+hnew)*m_realwidth + x+wnew)*stride + e] *= 0.25;
				}
			}
		}
		if(width > 1 && height > 1) // generate smaller image with the data just computed
			generateMipMap(texdata, wnew, hnew, wnew+width, hnew+height, wnew, height/2);
	}
	
  typename T::rectangular_type m_realtex; // the texture with the mipmap layers
	int m_realwidth; // the width of the new object

};


#endif
