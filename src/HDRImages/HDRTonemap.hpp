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
		ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		int width = m_node->width();
		int height = m_node->height();
		int stride = return_type::typesize;
		ShHostMemoryPtr luminance = new ShHostMemory(width * height * stride * sizeof(float));
		
		float* data = (float*)cursto->data();
		float* lum = (float*)luminance->hostStorage()->data();
		
		// compute the luminance of the image
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
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
					data[(y*width+x)*stride+e] = data[(y*width+x)*stride+e]*C;
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
