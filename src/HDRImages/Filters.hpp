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
#ifndef FILTERS_H
#define FILTERS_H

#include <iostream>
#include <string>
#include <math.h>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class GaussFilter : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef GaussFilter<typename T::rectangular_type> rectangular_type;
	
  GaussFilter(int sigma) : parent_type() { computeCoeff(sigma); }
	GaussFilter(int width, int sigma) : parent_type(width) { computeCoeff(sigma); }
	GaussFilter(int width, int height, int sigma) : parent_type(width, height) { computeCoeff(sigma); }
	GaussFilter(int width, int height, int depth, int sigma) : parent_type(width, height, depth) { computeCoeff(sigma); }
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
    return_type result = fillcast<return_type::typesize>(ShAttrib1f(0.0)); // clear
    for(int i=-m_filterWidth+1 ; i<m_filterWidth ; i++) {
      for(int j=-m_filterWidth+1 ; j<m_filterWidth ; j++) {
        result = mad(m_filterCoeff[abs(i)][abs(j)], (*bt)[tc+ShAttrib2f((float)i,(float)j)], result);
      }
    }
    return result;
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		const T *bt = this;
		return operator[](tc*size());
	}


private:
  void computeCoeff(int sigma) { // compute the coefficients
    m_filterWidth = 2*sigma+1;
    float* linearCoeff = new float[m_filterWidth];
    if(sigma != 0)
    {
      for(int i=0 ; i<m_filterWidth ; i++) {
       linearCoeff[i] = exp(-(float)i*i/(2*sigma*sigma)) / (sigma * 2.506628275); // create the coeff for 1 direction
      }
      m_filterCoeff = new float*[m_filterWidth];
      for(int i=0 ; i<m_filterWidth ; i++) {
        m_filterCoeff[i] = new float[m_filterWidth];
        for(int j=0 ; j<m_filterWidth ; j++) {
          m_filterCoeff[i][j] = linearCoeff[i]*linearCoeff[j]; // use the coeff in 1 direction to compute them for 2 directions
        }
      }
    }
    else
    {
       m_filterCoeff = new float*;
       m_filterCoeff[0] = new float;
       m_filterCoeff[0][0] = 1.0;
    }
  }
  
  float** m_filterCoeff;
  int m_filterWidth;
};


template<typename T>
class AnisDiff : public T {
public:
  typedef T parent_type;
	typedef typename T::return_type return_type;	
  typedef typename T::base_type base_type;
	typedef AnisDiff<typename T::rectangular_type> rectangular_type;
	
	AnisDiff() : parent_type() {}
	AnisDiff(int width) : parent_type(width) {}
	AnisDiff(int width, int height) : parent_type(width, height) {}
	AnisDiff(int width, int height, int depth) : parent_type(width, height, depth) {}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
    ShVector4f distance(0.27,0.67,0.06,0.0); // the distance function is a comparison between the luminances
    return_type tex = (*bt)[tc];
    return_type dist1 = (*bt)[tc+ShAttrib2f(0.0,1.0)] - tex;
    return_type dist2 = (*bt)[tc+ShAttrib2f(1.0,0.0)] - tex;
    return_type dist3 = (*bt)[tc-ShAttrib2f(0.0,1.0)] - tex;
    return_type dist4 = (*bt)[tc-ShAttrib2f(1.0,0.0)] - tex;
    ShAttrib1f v1 = pow(M_E, -0.2 * abs(distance | dist1));
    ShAttrib1f u1 = pow(M_E, -0.2 * abs(distance | dist2));
    ShAttrib1f v0 = pow(M_E, -0.2 * abs(distance | dist3));
    ShAttrib1f u0 = pow(M_E, -0.2 * abs(distance | dist4));
    return tex + 0.25 * (v1*dist1 + v0*dist3 + u1*dist2 + u0*dist4);
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
    return operator[](tc*size());
	}

};

#endif
