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

#define USE_GAUSSIAN 0

using namespace SH;

/** filters the data with a Gaussian filter
  * by doing 2 passes, one vertical and one horizontal
  */
ShHostMemoryPtr GaussianFilter(float* data, int width, int height, int stride, int sigma) {
  int filterWidth = 2*sigma+1; // the size of the filter depends on sigma
	float* horizontaldata = new float[width * height * stride];
	ShHostMemoryPtr filter = new ShHostMemory(width * height * stride * sizeof(float)); // create the result space
	float* verticaldata = (float*)filter->hostStorage()->data();
  
  // compute the coefficients
  float* gauss = new float[filterWidth];
	for(int i=0 ; i<filterWidth ; i++) {
    gauss[i] = exp(-(float)i*i/(2.0*sigma*sigma)) / (sqrt(2.0*M_PI)*sigma);
  }
  // horizontal
	for(int i=0 ; i<width ; i++) {
		for(int j=0 ; j<height ; j++) {
			for(int k=0 ; k<stride ; k++) {
        horizontaldata[(j*width+i)*stride+k] = 0.0;
        for(int d=-filterWidth+1 ; d<filterWidth ; d++) {
          int newi = i+d > width-1 ? width-1 : i+d;
          newi = newi < 0 ? 0 : newi; 
					horizontaldata[(j*width+i)*stride + k] += gauss[abs(d)]*data[(j*width+newi)*stride+k];
        }
  		}
  	}
	}
  // vertical
	for(int i=0 ; i<width ; i++) {
		for(int j=0 ; j<height ; j++) {
			for(int k=0 ; k<stride ; k++) {
        verticaldata[(j*width+i)*stride+k] = 0.0;
        for(int d=-filterWidth+1 ; d<filterWidth ; d++) {
          int newj = j+d > height-1 ? height-1 : j+d;
          newj = newj < 0 ? 0 : newj; 
				  verticaldata[(j*width+i)*stride + k] += gauss[abs(d)]*horizontaldata[(newj*width+i)*stride+k];
        }
			}
		}
	}
  return filter;
}


/** returns the anisotropic diffusion filter of the data
  */
ShHostMemoryPtr AnisotropicDiffFilter(float* data, int width, int height, int stride, int t) {
	ShHostMemoryPtr dist = new ShHostMemory(width * height * 2 * stride * sizeof(float));
	float* distdata = (float*)dist->hostStorage()->data();
	ShHostMemoryPtr filter = new ShHostMemory(width * height * stride * sizeof(float));
	float* filterdata = (float*)filter->hostStorage()->data();
  while(t>0) {
    // compute the distance between a point and its neighbour
    for(int i=0 ; i<width ; i++) {
      for(int j=0 ; j<height ; j++) {
        int iplus = i>width-2 ? width-1 : i+1;
        int jplus = j>height-2 ? height-1 : j+1;
        for(int k=0 ; k<stride ; k++) {
          distdata[(j*width+i)*2*stride+k] = data[(jplus*width+i)*stride+k] - data[(j*width+i)*stride+k];
          distdata[(j*width+i)*2*stride+stride+k] = data[(j*width+iplus)*stride+k] - data[(j*width+i)*stride+k];
        }
      }
    }
    // compute the luminance and the filter
    for(int i=0 ; i<width ; i++) {
      for(int j=0 ; j<height ; j++) {
        int iminus = i>1 ? i-1 : 0;
        int jminus = j>1 ? j-1 : 0;
        float v1 = 0.2 * abs(0.27*distdata[(j*width+i)*2*stride] +
                             0.67*distdata[(j*width+i)*2*stride+1] +
                             0.06*distdata[(j*width+i)*2*stride+2]);
        float u1 = 0.2 * abs(0.27*distdata[(j*width+i)*2*stride+stride] +
                             0.67*distdata[(j*width+i)*2*stride+stride+1] +
                             0.06*distdata[(j*width+i)*2*stride+stride+2]);
        float v0 = 0.2 * abs(0.27*distdata[(jminus*width+i)*2*stride] +
                             0.67*distdata[(jminus*width+i)*2*stride+1] +
                             0.06*distdata[(jminus*width+i)*2*stride+2]);
        float u0 = 0.2 * abs(0.27*distdata[(j*width+iminus)*2*stride+stride] +
                             0.67*distdata[(j*width+iminus)*2*stride+stride+1] +
                             0.06*distdata[(j*width+iminus)*2*stride+stride+2]);
        v1 = 1.0/(1.0+v1*v1);
        u1 = 1.0/(1.0+u1*u1);
        v0 = 1.0/(1.0+v0*v0);
        u0 = 1.0/(1.0+u0*u0);
        for(int k=0 ; k<3 ; k++) {
          filterdata[(j*width+i)*stride+k] = data[(j*width+i)*stride+k] +
                                             0.25 * (v1*distdata[(j*width+i)*2*stride+k] +
                                                     u1*distdata[(j*width+i)*2*stride+stride+k] -
                                                     v0*distdata[(jminus*width+i)*2*stride+k] -
                                                     u0*distdata[(j*width+iminus)*2*stride+stride+k]);
        }
      }
    }
    data = filterdata;
    t--;
  }
  return filter;
}

/** shrink an image by a factor 2
  * and then filter the result
  */
ShHostMemoryPtr Reduction(float *data, int width, int height, int stride, int sigma) {
	int doublewidth = width;
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
#if (USE_GAUSSIAN)
	return GaussianFilter(reducdata, width, height, stride, sigma); // filter the reduced image
#else
	return AnisotropicDiffFilter(reducdata, width, height, stride, sigma); // filter the reduced image
#endif
}

/** define the tone-mapping operator presented by M. Ashikhmin
  * the max and min of the luminance of a zone are computed
  * then used to compute a logarithm scaling factor
  * used on the data inside the zone used
  */
template<typename T>
class AshikhminToneMap : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef AshikhminToneMap<typename T::rectangular_type> rectangular_type;

	AshikhminToneMap(): parent_type() {}

	AshikhminToneMap(int width) : parent_type(width) {}

	AshikhminToneMap(int width, int height) : parent_type(width, height) {}

	AshikhminToneMap(int width, int height, int depth) : parent_type(width, height, depth) {}
	
	void updateToneMap() {
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
		ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		float* data = (float*)cursto->data();
    // reduce the image by a factor 2
		ShHostMemoryPtr luminance = Reduction(data, width, height, stride, 1);
		width /=2;
		height /=2;
		float* lum = (float*)luminance->hostStorage()->data();
		
		// compute the luminance of the image
		int scalingFactor = m_node->width()/width;
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				lum[(y*width + x)*stride] = 0.27*lum[(y*width + x)*stride] + 0.67*lum[(y*width + x)*stride + 1] + 0.06*lum[(y*width +x )*stride +2];
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
				while(abs((oldlum - newlum)/oldlum) < 0.5 && filter <= 20) { // stop if there is a too big contrast
					int cmpt = 0;
					oldlum = newlum;
					newlum = 0.0;
					for(int i=-filter ; i<filter ; i++) {
						for(int j=-filter ; j<filter ; j++) {
							if(x+i>0 && x+i<width && y+j>0 && y+j<height) {
								newlum += lum[((y+j)*width + x+i)*stride]; // add the new points
								cmpt++;
							}
						}
					}
					newlum /= cmpt; // to get the average
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

    // use the factor computed to scale the value
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				float C = (lum[(y*width+x)*stride+2] - minlum) / (maxlum - minlum);
				C /= lum[(y*width+x)*stride+1];
				for(int i=scalingFactor*x; i<scalingFactor*(x+1) ; i++) {
				  for(int j=scalingFactor*y; j<scalingFactor*(y+1) ; j++) {
    				for(int e=0 ; e<stride ; e++) {
							data[(j*m_node->width()+i)*stride+e] *= C;
						}
					}
				}
			}
		}
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
		const T *bt = this;
		return (*bt)[tc]; // nothing special here
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		const T *bt = this;
		return (*bt)(tc); // nothing special here
	}
};


/** define the tone-mapping operator introduced by Reinhard
  * the data are scaled by a simple coefficient
  * depending on the average luminosity of a disk
  */
template<typename T>
class ReinhardToneMap : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef ReinhardToneMap<typename T::rectangular_type> rectangular_type;

	ReinhardToneMap() : parent_type()	{}

	ReinhardToneMap(int width) : parent_type(width)	{}

	ReinhardToneMap(int width, int height) : parent_type(width, height)	{}

	ReinhardToneMap(int width, int height, int depth) : parent_type(width, height, depth)	{}

	void updateToneMap() {
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
		ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		float* data = (float*)cursto->data();
    // reduce the image by a factor 2
    ShHostMemoryPtr reduction =  Reduction(data, width, height, stride, 1);
    float *data2 = (float*)reduction->hostStorage()->data();
    width /= 2;
    height /= 2;
		int scalingFactor = m_node->width()/width;
		ShHostMemoryPtr luminance = new ShHostMemory(width * height * stride * sizeof(float));
		float *lum = (float*)luminance->hostStorage()->data();
		   
		// the luminance of the image
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				lum[(y*width + x)*stride] = 0.27*data2[(y*width + x)*stride] +
																		0.67*data2[(y*width + x)*stride + 1] +
																		0.06*data2[(y*width + x)*stride + 2];
			}
		}
    // for each point, compute the coeff and scale with it
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				float V1 = 100.0;
				float V2 = 0.0;
				float s = 1.0;
        float a = 0.30;
				while(abs((V1-V2)/(256*a/(s*s)+V1)) > 0.05 && s < 43.0) { // stop if there is a too big contrast
					V1=0.0;
					V2=0.0;
					float s2 = s*s;
          // compute the luminance of the zones
					for(int i=-(int)s ; i<=(int)s ; i++) {
						for(int j=-(int)s ; j<=(int)s ; j++) {
							if(x+i>=0 && x+i<width && y+j>=0 && y+j<height) {
								float k = i*i+j*j;
								V1 += lum[((y+j)*width + x+i)*stride] * exp(-k/(0.1225*s2)) / (M_PI*(0.1225*s2));
								V2 += lum[((y+j)*width + x+i)*stride] * exp(-k/(0.3136*s2)) / (M_PI*(0.3136*s2));
							}
						}
					}
					s *= 1.6;
				}
        // use the coeff found to scale the data
				float Ld = 1.0 / (1.0 + V1) * 1.0 / a;
				for(int i=scalingFactor*x; i<scalingFactor*(x+1) ; i++) {
					for(int j=scalingFactor*y; j<scalingFactor*(y+1) ; j++) {
    				data[(j*m_node->width() + i)*stride] *= Ld;
	    			data[(j*m_node->width() + i)*stride + 1] *= Ld;
		    		data[(j*m_node->width() + i)*stride + 2] *= Ld;
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

};

#endif
