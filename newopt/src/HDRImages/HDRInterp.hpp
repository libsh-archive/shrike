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
#ifndef HDRINTERPOLATE_H
#define HDRINTERPOLATE_H

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class LinInterp : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef LinInterp<typename T::rectangular_type> rectangular_type;
	
	LinInterp() : parent_type()
	{}

	LinInterp(int width) : parent_type(width)
	{}

	LinInterp(int width, int height) : parent_type(width, height)
	{}

	LinInterp(int width, int height, int depth) : parent_type(width, height, depth)
	{}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f dtc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib2f oneone(1.0,1.0);
		// clamp coordinates to the texture size to avoid artifacts with mip-mapping
		ShAttrib2f limit = size() - oneone;
		ShAttrib2f dtc2 = oneone-dtc;
		ShAttrib2f umin = SH::min(limit,u+oneone);
		return dtc2(0) * (dtc2(1)*(*bt)[u] + dtc(1)*(*bt)[ShAttrib2f(u(0),umin(1))]) +
					 dtc(0) * (dtc2(1)*(*bt)[ShAttrib2f(umin(0),u(1))] + dtc(1)*(*bt)[umin]);
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());			
	}
};


template<typename T>
class BicubicInterp : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef BicubicInterp<typename T::rectangular_type> rectangular_type;
	
	BicubicInterp() : parent_type()
	{}

	BicubicInterp(int width) : parent_type(width)
	{}

	BicubicInterp(int width, int height) : parent_type(width, height)
	{}

	BicubicInterp(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f fractc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib2f oneone(1.0,1.0);
		
		ShAttrib4f dtc1 = ShConstAttrib4f(1.0, 1.0, 0.0, 0.0) + fractc(0, 1, 0, 1);
		ShAttrib4f dtc2 = ShConstAttrib4f(1.0, 1.0, 2.0, 2.0) - fractc(0, 1, 0, 1);
		ShAttrib4f dtc12 = dtc1 * dtc1;
		ShAttrib4f dtc13 = dtc12 * dtc1;
		ShAttrib4f dtc22 = dtc2 * dtc2;
		ShAttrib4f dtc23 = dtc22 * dtc2;

		// compute the coefficients of the 16 pixels used
		ShAttrib2f h_1 = -0.5*dtc13(0,1);
		h_1 = mad(2.0,oneone,h_1);
		h_1 = mad(-4.0,dtc1(0,1),h_1);
		h_1 = mad(2.5,dtc12(0,1),h_1);
		ShAttrib2f h0 = oneone;
		h0 = mad(-2.5,dtc12(2,3),h0);
		h0 = mad(1.5,dtc13(2,3),h0);
		ShAttrib2f h1 = oneone;
		h1 = mad(-2.5,dtc22(0,1),h1);
		h1 = mad(1.5,dtc23(0,1),h1);
		ShAttrib2f h2 = -0.5*dtc23(2,3);
		h2 = mad(2.0,oneone,h2);
		h2 = mad(-4.0,dtc2(2,3),h2);
		h2 = mad(2.5,dtc22(2,3),h2);

		// clamp coordinates to the texture size to avoid artifacts with mip-mapping
		ShAttrib2f limit = size() - oneone;
		ShAttrib2f u_1 = u - oneone;
		ShAttrib4f u12 = u(0,1,0,1) + ShConstAttrib4f(1.0,1.0,2.0,2.0);
		u_1 = SH::max(ShAttrib2f(0.0,0.0),u_1);
		u12 = SH::min(limit(0,1,0,1),u12);

		return h_1(0) * (h_1(1) * (*bt)[u_1] + h0(1) * (*bt)[ShAttrib2f(u_1(0),u(1))] +
					 					 h1(1) * (*bt)[ShAttrib2f(u_1(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u_1(0),u12(3))]) +
					 h0(0) * (h_1(1) * (*bt)[ShAttrib2f(u(0),u_1(1))] + h0(1) * (*bt)[u] +
										h1(1) * (*bt)[ShAttrib2f(u(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u(0),u12(3))]) +
					 h1(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(0),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(0),u(1))] +
										h1(1) * (*bt)[u12(0,1)] + h2(1) * (*bt)[u12(0,3)]) +
					 h2(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(2),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(2),u(1))] +
					 					h1(1) * (*bt)[u12(2,1)] + h2(1) * (*bt)[u12(2,3)]);
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());
	}
};

template<typename T>
class CubicBSplineInterp : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef CubicBSplineInterp<typename T::rectangular_type> rectangular_type;
	
	CubicBSplineInterp() : parent_type()
	{}

	CubicBSplineInterp(int width) : parent_type(width)
	{}

	CubicBSplineInterp(int width, int height) : parent_type(width, height)
	{}

	CubicBSplineInterp(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f fractc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib2f oneone(1.0,1.0);
		
		ShAttrib4f dtc1 = ShConstAttrib4f(1.0, 1.0, 0.0, 0.0) + fractc(0, 1, 0, 1);
		ShAttrib4f dtc2 = ShConstAttrib4f(1.0, 1.0, 2.0, 2.0) - fractc(0, 1, 0, 1);
		ShAttrib4f dtc12 = dtc1 * dtc1;
		ShAttrib4f dtc13 = dtc12 * dtc1;
		ShAttrib4f dtc22 = dtc2 * dtc2;
		ShAttrib4f dtc23 = dtc22 * dtc2;

		// compute the coefficients of the 16 pixels used
		ShAttrib2f h_1 = -dtc13(0,1);
		h_1 = mad(8.0,oneone,h_1);
		h_1 = mad(-12.0,dtc1(0,1),h_1);
		h_1 = mad(6.0,dtc12(0,1),h_1);
		ShAttrib2f h0 = 4.0*oneone;
		h0 = mad(-6.0,dtc12(2,3),h0);
		h0 = mad(3.0,dtc13(2,3),h0);
		ShAttrib2f h1 = 4.0*oneone;
		h1 = mad(-6.0,dtc22(0,1),h1);
		h1 = mad(3.0,dtc23(0,1),h1);
		ShAttrib2f h2 = -dtc23(2,3);
		h2 = mad(8.0,oneone,h2);
		h2 = mad(-12.0,dtc2(2,3),h2);
		h2 = mad(6.0,dtc22(2,3),h2);

		// clamp coordinates to the texture size to avoid artifacts with mip-mapping
		ShAttrib2f limit = size() - oneone;
		ShAttrib2f u_1 = u - oneone;
		ShAttrib4f u12 = u(0,1,0,1) + ShConstAttrib4f(1.0,1.0,2.0,2.0);
		u_1 = SH::max(ShAttrib2f(0.0,0.0),u_1);
		u12 = SH::min(limit(0,1,0,1),u12);

		return_type result = h_1(0) * (h_1(1) * (*bt)[u_1] + h0(1) * (*bt)[ShAttrib2f(u_1(0),u(1))] +
												 					 h1(1) * (*bt)[ShAttrib2f(u_1(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u_1(0),u12(3))]) +
												 h0(0) * (h_1(1) * (*bt)[ShAttrib2f(u(0),u_1(1))] + h0(1) * (*bt)[u] +
																	 h1(1) * (*bt)[ShAttrib2f(u(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u(0),u12(3))]) +
												 h1(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(0),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(0),u(1))] +
															  	 h1(1) * (*bt)[u12(0,1)] + h2(1) * (*bt)[ShAttrib2f(u12(0),u12(3))]) +
												 h2(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(2),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(2),u(1))] +
					 												 h1(1) * (*bt)[ShAttrib2f(u12(2),u12(1))] + h2(1) * (*bt)[u12(2,3)]);
		return 0.027777778*result; // 0.027777778 = 1/36
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());
	}
};


/*
// another B-Spline interpolation function
template<typename T>
class BSplineInterp2 : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef BSplineInterp2<typename T::rectangular_type> rectangular_type;
	
	BSplineInterp2() : parent_type()
	{}

	BSplineInterp2(int width) : parent_type(width)
	{}

	BSplineInterp2(int width, int height) : parent_type(width, height)
	{}

	BSplineInterp2(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f dtc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib4f dx = ShConstAttrib4f(1.0, 2.0, 3.0, 4.0) - dtc(0, 0, 0, 0);
		dx *= dx * dx;

		ShAttrib3f d1 = dx(1,2,3) - 4.0*dx(0, 1, 2);
		d1(1,2) = mad(6.0, dx(0,1), d1(1,2));
		d1(2) = mad(-4.0, dx(0), d1(2));
		
		ShAttrib4f dy = ShConstAttrib4f(0.0, 1.0, 2.0, 3.0) + dtc(1, 1, 1, 1);
		dy *= dy * dy;
		ShAttrib3f d2 = dy(1,2,3) - 4.0*dy(0,1,2);
		d2(1,2) = mad(6.0, dy(0,1), d2(1,2));
		d2(2) = mad(-4.0, dy(0), d2(2));
		
		ShAttrib2f limit(size()(0)-1.0,size()(1)-1.0);
		ShAttrib2f u_1(u(0)-1.0,u(1)-1.0);
		ShAttrib2f u1(u(0)+1.0,u(1)+1.0);
		ShAttrib2f u2(u(0)+2.0,u(1)+2.0);
		u_1 = SH::max(ShAttrib2f(0.0,0.0),u_1);
		u1 = SH::min(limit,u1);
		u2 = SH::min(limit,u2);
		
		return_type result =	((*bt)[u_1] * dx(0) + (*bt)[ShAttrib2f(u(0),u_1(1))] * d1(0) +
													 (*bt)[ShAttrib2f(u1(0),u_1(1))] * d1(1) + (*bt)[ShAttrib2f(u2(0),u_1(1))] * d1(2)) * d2(2) +
													((*bt)[ShAttrib2f(u_1(0),u(1))] * dx(0) + (*bt)[u] * d1(0) +
													 (*bt)[ShAttrib2f(u1(0),u(1))] * d1(1) + (*bt)[ShAttrib2f(u2(0),u(1))] * d1(2)) * d2(1) +
													((*bt)[ShAttrib2f(u_1(0),u1(1))] * dx(0) + (*bt)[ShAttrib2f(u(0),u1(1))] * d1(0) +
													 (*bt)[u1] * d1(1) + (*bt)[ShAttrib2f(u2(0),u1(1))] * d1(2)) * d2(0) +
													((*bt)[ShAttrib2f(u_1(0),u2(1))] * dx(0) + (*bt)[ShAttrib2f(u(0),u(1))] * d1(0) +
													 (*bt)[ShAttrib2f(u1(0),u2(1))] * d1(1) + (*bt)[u2] * d1(2)) * dy(0);
		return 0.027777778*result; // 0.027777778 = 1/36
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());
	}
};
*/

template<typename T>
class CardinalSplineInterp : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef CardinalSplineInterp<typename T::rectangular_type> rectangular_type;
	
	CardinalSplineInterp() : parent_type()
	{}

	CardinalSplineInterp(int width) : parent_type(width)
	{}

	CardinalSplineInterp(int width, int height) : parent_type(width, height)
	{}

	CardinalSplineInterp(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	void updateCardSpline() {
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
    ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
    ShHostMemoryPtr newmem = new ShHostMemory(width * height * stride * sizeof(float));
		T tmptex;
    tmptex.memory(newmem);
    tmptex.size(width, height);
		float* olddata = (float*)cursto->data();
		float* newdata = (float*)newmem->hostStorage()->data();
		float zp = -0.171572875;
		int k0 = 10;
		int c0 = 8;
		for (int e = 0; e < stride; e++) {
		// clear the data
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					newdata[(y*width)*stride+e] = 0.0;
				}
			}
		// 1st step: boundary condition
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < k0; x++) {
  				newdata[(y*width)*stride + e] +=	pow(zp,x) * olddata[(y*width + x)*stride + e];
        }
			}
		// 2nd step: forward recursion
  		for (int y = 0; y < height; y++) {
   		 	for (int x = 1; x < width; x++) {
  				newdata[(y*width + x)*stride + e] =	olddata[(y*width + x)*stride + e] + zp*newdata[(y*width + x-1)*stride + e];
				}
			}
		// 3rd step: boundary condition
  		for (int y = 0; y < height; y++) {
 				newdata[(y*width + width-1)*stride + e] =	-zp/(1.0-zp*zp)*(2.0*newdata[(y*width + width-1)*stride + e] - olddata[(y*width + width-1)*stride + e]);
			}
		// 4th step: backward recursion
  		for (int y = 0; y < height; y++) {
   		 	for (int x = width-2; x >= 0; x--) {
  				newdata[(y*width + x)*stride + e] =	zp*(newdata[(y*width + x+1)*stride + e] - newdata[(y*width + x)*stride + e]);
				}
			}
		// 5th step: scale by a constant
  		for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					newdata[(y*width + x)*stride +e] *= c0;
				}
			}
		
		// the other direction
		// clear the data
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					olddata[(y*width)*stride+e] = 0.0;
				}
			}
		// 1st step: boundary condition
   	 	for (int x = 0; x < width; x++) {
	  		for (int y = 0; y < k0; y++) {
  				olddata[x*stride + e] +=	pow(zp,x) * newdata[(y*width + x)*stride + e];
        }
			}
		// 2nd step: forward recursion
   		for (int x = 0; x < width; x++) {
  			for (int y = 1; y < height; y++) {
  				olddata[(y*width + x)*stride + e] =	newdata[(y*width + x)*stride + e] + zp*olddata[((y-1)*width + x)*stride + e];
				}
			}
		// 3rd step: boundary condition
  		for (int x = 0; x < width; x++) {
 				olddata[((height-1)*width + x)*stride + e] =	-zp/(1.0-zp*zp)*(2.0*olddata[((height-1)*width + x)*stride + e] - newdata[((height-1)*width + x)*stride + e]);
			}
		// 4th step: backward recursion
   		for (int x = 0; x < width; x++) {
  			for (int y = height-2; y >= 0; y--) {
  				olddata[(y*width + x)*stride + e] =	zp*(olddata[((y+1)*width + x)*stride + e] - olddata[(y*width + x)*stride + e]);
				}
			}
		// 5th step: scale by a constant
  		for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					olddata[(y*width + x)*stride +e] *= c0;
				}
			}
		}
		// no step 6: update because the data used for the second pass are the data to output
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f fractc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib2f oneone(1.0,1.0);
		
		ShAttrib4f dtc1 = ShConstAttrib4f(1.0, 1.0, 0.0, 0.0) + fractc(0, 1, 0, 1);
		ShAttrib4f dtc2 = ShConstAttrib4f(1.0, 1.0, 2.0, 2.0) - fractc(0, 1, 0, 1);
		ShAttrib4f dtc12 = dtc1 * dtc1;
		ShAttrib4f dtc13 = dtc12 * dtc1;
		ShAttrib4f dtc22 = dtc2 * dtc2;
		ShAttrib4f dtc23 = dtc22 * dtc2;

		// compute the coefficients of the 16 pixels used
		ShAttrib2f h_1 = -dtc13(0,1);
		h_1 = mad(8.0,oneone,h_1);
		h_1 = mad(-12.0,dtc1(0,1),h_1);
		h_1 = mad(6.0,dtc12(0,1),h_1);
		ShAttrib2f h0 = 4.0*oneone;
		h0 = mad(-6.0,dtc12(2,3),h0);
		h0 = mad(3.0,dtc13(2,3),h0);
		ShAttrib2f h1 = 4.0*oneone;
		h1 = mad(-6.0,dtc22(0,1),h1);
		h1 = mad(3.0,dtc23(0,1),h1);
		ShAttrib2f h2 = -dtc23(2,3);
		h2 = mad(8.0,oneone,h2);
		h2 = mad(-12.0,dtc2(2,3),h2);
		h2 = mad(6.0,dtc22(2,3),h2);

		// clamp coordinates to the texture size to avoid artifacts with mip-mapping
		ShAttrib2f limit = size() - oneone;
		ShAttrib2f u_1 = u - oneone;
		ShAttrib4f u12 = u(0,1,0,1) + ShConstAttrib4f(1.0,1.0,2.0,2.0);
		u_1 = SH::max(ShAttrib2f(0.0,0.0),u_1);
		u12 = SH::min(limit(0,1,0,1),u12);

		return_type result = h_1(0) * (h_1(1) * (*bt)[u_1] + h0(1) * (*bt)[ShAttrib2f(u_1(0),u(1))] +
												 					 h1(1) * (*bt)[ShAttrib2f(u_1(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u_1(0),u12(3))]) +
												 h0(0) * (h_1(1) * (*bt)[ShAttrib2f(u(0),u_1(1))] + h0(1) * (*bt)[u] +
																	 h1(1) * (*bt)[ShAttrib2f(u(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u(0),u12(3))]) +
												 h1(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(0),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(0),u(1))] +
															  	 h1(1) * (*bt)[u12(0,1)] + h2(1) * (*bt)[ShAttrib2f(u12(0),u12(3))]) +
												 h2(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(2),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(2),u(1))] +
					 												 h1(1) * (*bt)[ShAttrib2f(u12(2),u12(1))] + h2(1) * (*bt)[u12(2,3)]);
		return 0.027777778*result; // 0.027777778 = 1/36
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());
	}

};

template<typename T>
class SmoothSplineInterp : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef SmoothSplineInterp<typename T::rectangular_type> rectangular_type;
	
	SmoothSplineInterp() : parent_type()
	{}

	SmoothSplineInterp(int width) : parent_type(width)
	{}

	SmoothSplineInterp(int width, int height) : parent_type(width, height)
	{}

	SmoothSplineInterp(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	void updateSmoothSpline(float lambda) {
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
    ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
    ShHostMemoryPtr newmem = new ShHostMemory(width * height * stride * sizeof(float));
		T tmptex;
    tmptex.memory(newmem);
    tmptex.size(width, height);
		float* olddata = (float*)cursto->data();
		float* newdata = (float*)newmem->hostStorage()->data();
		lambda /= -100.0;
		float zp = 1.0 + 0.5/lambda - 0.5*sqrt(1.0+4*lambda)/lambda;
		int k0 = 10;
		float c0 = -1.0/lambda;
		for (int e = 0; e < stride; e++) {
		// clear the data
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					newdata[(y*width)*stride+e] = 0.0;
				}
			}
		// 1st step: boundary condition
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < k0; x++) {
  				newdata[(y*width)*stride + e] +=	pow(zp,x) * olddata[(y*width + x)*stride + e];
        }
			}
		// 2nd step: forward recursion
  		for (int y = 0; y < height; y++) {
   		 	for (int x = 1; x < width; x++) {
  				newdata[(y*width + x)*stride + e] =	olddata[(y*width + x)*stride + e] + zp*newdata[(y*width + x-1)*stride + e];
				}
			}
		// 3rd step: boundary condition
  		for (int y = 0; y < height; y++) {
 				newdata[(y*width + width-1)*stride + e] =	-zp/(1.0-zp*zp)*(2.0*newdata[(y*width + width-1)*stride + e] - olddata[(y*width + width-1)*stride + e]);
			}
		// 4th step: backward recursion
  		for (int y = 0; y < height; y++) {
   		 	for (int x = width-2; x >= 0; x--) {
  				newdata[(y*width + x)*stride + e] =	zp*(newdata[(y*width + x+1)*stride + e] - newdata[(y*width + x)*stride + e]);
				}
			}
		// 5th step: scale by a constant
  		for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					newdata[(y*width + x)*stride +e] *= c0;
				}
			}
		
		// the other direction
		// clear the data
	  	for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					olddata[(y*width)*stride+e] = 0.0;
				}
			}
		// 1st step: boundary condition
   	 	for (int x = 0; x < width; x++) {
	  		for (int y = 0; y < k0; y++) {
  				olddata[x*stride + e] +=	pow(zp,x) * newdata[(y*width + x)*stride + e];
        }
			}
		// 2nd step: forward recursion
   		for (int x = 0; x < width; x++) {
  			for (int y = 1; y < height; y++) {
  				olddata[(y*width + x)*stride + e] =	newdata[(y*width + x)*stride + e] + zp*olddata[((y-1)*width + x)*stride + e];
				}
			}
		// 3rd step: boundary condition
  		for (int x = 0; x < width; x++) {
 				olddata[((height-1)*width + x)*stride + e] =	-zp/(1.0-zp*zp)*(2.0*olddata[((height-1)*width + x)*stride + e] - newdata[((height-1)*width + x)*stride + e]);
			}
		// 4th step: backward recursion
   		for (int x = 0; x < width; x++) {
  			for (int y = height-2; y >= 0; y--) {
  				olddata[(y*width + x)*stride + e] =	zp*(olddata[((y+1)*width + x)*stride + e] - olddata[(y*width + x)*stride + e]);
				}
			}
		// 5th step: scale by a constant
  		for (int y = 0; y < height; y++) {
   	 		for (int x = 0; x < width; x++) {
					olddata[(y*width + x)*stride +e] *= c0;
				}
			}
		}
		// no step 6: update because the data used for the second pass are the data to output
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		ShAttrib2f fractc = frac(tc);
		ShAttrib2f u = floor(tc);
		ShAttrib2f oneone(1.0,1.0);
		
		ShAttrib4f dtc1 = ShConstAttrib4f(1.0, 1.0, 0.0, 0.0) + fractc(0, 1, 0, 1);
		ShAttrib4f dtc2 = ShConstAttrib4f(1.0, 1.0, 2.0, 2.0) - fractc(0, 1, 0, 1);
		ShAttrib4f dtc12 = dtc1 * dtc1;
		ShAttrib4f dtc13 = dtc12 * dtc1;
		ShAttrib4f dtc22 = dtc2 * dtc2;
		ShAttrib4f dtc23 = dtc22 * dtc2;

		// compute the coefficients of the 16 pixels used
		ShAttrib2f h_1 = -dtc13(0,1);
		h_1 = mad(8.0,oneone,h_1);
		h_1 = mad(-12.0,dtc1(0,1),h_1);
		h_1 = mad(6.0,dtc12(0,1),h_1);
		ShAttrib2f h0 = 4.0*oneone;
		h0 = mad(-6.0,dtc12(2,3),h0);
		h0 = mad(3.0,dtc13(2,3),h0);
		ShAttrib2f h1 = 4.0*oneone;
		h1 = mad(-6.0,dtc22(0,1),h1);
		h1 = mad(3.0,dtc23(0,1),h1);
		ShAttrib2f h2 = -dtc23(2,3);
		h2 = mad(8.0,oneone,h2);
		h2 = mad(-12.0,dtc2(2,3),h2);
		h2 = mad(6.0,dtc22(2,3),h2);

		// clamp coordinates to the texture size to avoid artifacts with mip-mapping
		ShAttrib2f limit = size() - oneone;
		ShAttrib2f u_1 = u - oneone;
		ShAttrib4f u12 = u(0,1,0,1) + ShConstAttrib4f(1.0,1.0,2.0,2.0);
		u_1 = SH::max(ShAttrib2f(0.0,0.0),u_1);
		u12 = SH::min(limit(0,1,0,1),u12);

		return_type result = h_1(0) * (h_1(1) * (*bt)[u_1] + h0(1) * (*bt)[ShAttrib2f(u_1(0),u(1))] +
												 					 h1(1) * (*bt)[ShAttrib2f(u_1(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u_1(0),u12(3))]) +
												 h0(0) * (h_1(1) * (*bt)[ShAttrib2f(u(0),u_1(1))] + h0(1) * (*bt)[u] +
																	 h1(1) * (*bt)[ShAttrib2f(u(0),u12(1))] + h2(1) * (*bt)[ShAttrib2f(u(0),u12(3))]) +
												 h1(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(0),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(0),u(1))] +
															  	 h1(1) * (*bt)[u12(0,1)] + h2(1) * (*bt)[ShAttrib2f(u12(0),u12(3))]) +
												 h2(0) * (h_1(1) * (*bt)[ShAttrib2f(u12(2),u_1(1))] + h0(1) * (*bt)[ShAttrib2f(u12(2),u(1))] +
					 												 h1(1) * (*bt)[ShAttrib2f(u12(2),u12(1))] + h2(1) * (*bt)[u12(2,3)]);
		return 0.027777778*result; // 0.027777778 = 1/36
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		return operator[](tc*size());
	}

};

#endif