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
#ifndef HDRTONEMAPPING_HPP
#define HDRTONEMAPPING_HPP

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class ToneMap : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef ToneMap<typename T::rectangular_type> rectangular_type;

	ToneMap() : parent_type()
	{}

	ToneMap(int width) : parent_type(width)
	{}

	ToneMap(int width, int height) : parent_type(width, height)
	{}

	ToneMap(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	
	// compute a tone mapping factor
	// see Ashikhmin's paper "A Tone Mapping Algorithm for High Contrast Images" for details
	// a texture is used to save all the different values computed for each pixel
	void updateToneMap() {
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
		ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		float* data = (float*)cursto->data();
		ShHostMemoryPtr luminance = new ShHostMemory(width * height * stride * sizeof(float));
		/*
		ShHostMemoryPtr luminance = Reduction(data, width, height);
		width /=2;
		height /=2;
		float* lum = (float*)luminance->hostStorage()->data();
		luminance = Reduction(lum, width, height);
		width /=2;
		height /=2;
		*/
		float *lum = (float*)luminance->hostStorage()->data();
		// compute the luminance of the image
		int scalingFactor = m_node->width()/width;
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				//lum[(y*width + x)*stride] = 0.2126*lum[(y*width + x)*stride] + 0.7152*lum[(y*width + x)*stride + 1] + 0.0722*lum[(y*width +x )*stride +2];
				lum[(y*width + x)*stride] = 0.2126*data[(y*width + x)*stride] + 0.7152*data[(y*width + x)*stride + 1] + 0.0722*data[(y*width +x )*stride +2];
			}
		}
		float maxlum=-10.0, minlum=10.0;
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				lum[(y*width + x)*stride +1] = 0.0;
				float oldlum = lum[(y*width + x)*stride];
				float newlum = oldlum;
				int filter = 1;
				// compute the local adapatation level
				while(abs((oldlum - newlum)/oldlum) < 0.5 && filter < 40) { // stop if there is a too big contrast
					int cmpt = 0;
					oldlum = newlum;
					newlum = 0.0;
					for(int i=-filter ; i<filter ; i++) {
						for(int j=-filter ; j<filter ; j++) {
							if(x+i>0 && x+i<width && y+j>0 && y+j<height) {
								newlum += lum[((y+j)*width + x+i)*stride];
								cmpt++;
							}
						}
					}
					newlum /= cmpt;
					filter *= 2; // add more pixels too get the maximum size
				}
				// compute the capacity function for the min and the max value of the luminance
				lum[(y*width +x)*stride +1] = oldlum;
				if(lum[(y*width +x)*stride +1] > maxlum)
					maxlum = lum[(y*width +x)*stride +1];
				if(lum[(y*width +x)*stride +1] < minlum)
					minlum = lum[(y*width +x)*stride +1];
				if(lum[(y*width+x)*stride+1] < 0.0034)
					lum[(y*width+x)*stride+2] = lum[(y*width+x)*stride+1]/0.0014;
				else if(lum[(y*width+x)*stride+1] < 1.0)
					lum[(y*width+x)*stride+2] = 2.4483 + log(lum[(y*width+x)*stride+1]/0.0034)/0.4027;
				else if(lum[(y*width+x)*stride+1] < 7.2444)
					lum[(y*width+x)*stride+2] = 16.5630 + (lum[(y*width+x)*stride+1]-1.0)/0.4027;
				else
					lum[(y*width+x)*stride+2] = 32.0693 + log(lum[(y*width+x)*stride+1]/7.2444)/0.0556;
			}
		}
		// compute the capacity function
		if(minlum < 0.0034)
			minlum = minlum/0.0014;
		else if(minlum < 1.0)
			minlum = 2.4483 + log(minlum/0.0034)/0.4027;
		else if(minlum < 7.2444)
			minlum = 16.5630 + (minlum-1.0)/0.4027;
		else
			minlum = 32.0693 + log(minlum/7.2444)/0.0556;
		if(maxlum < 0.0034)
			maxlum = maxlum/0.0014;
		else if(maxlum < 1.0)
			maxlum = 2.4483 + log(maxlum/0.0034)/0.4027;
		else if(maxlum< 7.2444)
			maxlum = 16.5630 + (maxlum-1.0)/0.4027;
		else
			maxlum = 32.0693 + log(maxlum/7.2444)/0.0556;
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				float C = (lum[(y*width+x)*stride+2] - minlum) / (maxlum - minlum);
				C /= lum[(y*width+x)*stride+1];
				for(int e=0 ; e<stride ; e++) {
					for(int i=scalingFactor*x; i<scalingFactor*(x+1) ; i++) {
						for(int j=scalingFactor*y; j<scalingFactor*(y+1) ; j++) {
							data[(j*m_node->width()+i)*stride+e] *= C;
						}
					}
				}
			}
		}
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		return (*bt)[tc];
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		const T *bt = this;
		return (*bt)(tc);
	}

private:
	// use nearest neighbour the reduce the size of an image by 2
	ShHostMemoryPtr Reduction(float *data, int width, int height) {
		int doublewidth = width;
		int stride = return_type::typesize;
		
		width /= 2;
		height /= 2;
		ShHostMemoryPtr reduc = new ShHostMemory(width * height * stride * sizeof(float));
		float* reducdata = (float*)reduc->hostStorage()->data();
	
		for(int i=0 ; i<width ; i++) {
			for(int j=0 ; j<height ; j++) {
				for(int k=0 ; k<stride ; k++) {
					reducdata[(j*width+i)*stride+k] = 0.25*(data[(2*j*doublewidth+2*i)*stride+k] + data[((2*j+1)*doublewidth+2*i)*stride+k] +
																								  data[(2*j*doublewidth+2*i+1)*stride+k] + data[((2*j+1)*doublewidth+2*i+1)*stride+k]);
				}
			}
		}
		GaussianFilter(reducdata, width, height); // filter the reduced image
		return reduc;
	}

	// apply a Gaussian filer on an image
	void GaussianFilter(float* data, int width, int height) {
		int stride = return_type::typesize;
		float gauss[4] = {0.053991, 0.241971, 0.398942, 0.241971};
		for(int i=0 ; i<width ; i++) {
			for(int j=0 ; j<height ; j++) {
				for(int k=0 ; k<stride ; k++) {
					int min1 = i-1 < 0 ? 0 : i-1;
					int min2 = i-2 < 0 ? 0 : i-2;
					int max1 = i+1 > width-1 ? width-1 : i+1;
					int max2 = i+2 > width-1 ? width-1 : i+2;
					data[(j*width+i)*stride + k ] = gauss[0]*data[(j*width+min2)*stride+k]+
		  																		gauss[1]*data[(j*width+min1)*stride+k]+
																					gauss[2]*data[(j*width+i)*stride+k]+
																					gauss[1]*data[(j*width+max1)*stride+k]+
																					gauss[0]*data[(j*width+max2)*stride+k];
				}
			}
		}
		for(int i=0 ; i<width ; i++) {
			for(int j=0 ; j<height ; j++) {
				for(int k=0 ; k<stride ; k++) {
					int min1 = j-1 < 0 ? 0 : j-1;
					int min2 = j-2 < 0 ? 0 : j-2;
					int max1 = j+1 > height-1 ? height-1 : j+1;
					int max2 = j+2 > height-1 ? height-1 : j+2;
					data[(j*width+i)*stride + k ] = gauss[0]*data[(min2*width+i)*stride+k]+
																					gauss[1]*data[(min1*width+i)*stride+k]+
																					gauss[2]*data[(j*width+i)*stride+k]+
																					gauss[1]*data[(max1*width+i)*stride+k]+
																					gauss[0]*data[(max2*width+i)*stride+k];
				}
			}
		}
	}
};


#endif
