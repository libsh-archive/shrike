#ifndef HDRMIPMAPPING_HPP
#define HDRMIPMAPPING_HPP

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T>
class MipMap : public T {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef MipMap<typename T::rectangular_type> rectangular_type;

	MipMap() : parent_type()
	{}

	MipMap(int width) : parent_type(width)
	{}

	MipMap(int width, int height) : parent_type(width, height)
	{}

	MipMap(int width, int height, int depth) : parent_type(width, height, depth)
	{}

	// create a new texture to save the original texture and all the mipmap levels
	void updateMipMap() {
		int width = m_node->width();
		int halfwidth = width/2;
		int height = m_node->height();
		int stride = return_type::typesize;
    ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		m_realwidth = width + halfwidth + (width % 2); // width of the new texture
    ShHostMemoryPtr newmem = new ShHostMemory(m_realwidth * height * stride * sizeof(float));
    m_realtex.memory(newmem);
    m_realtex.size(m_realwidth, m_node->height());
		
		float* olddata = (float*)cursto->data();
		float* newdata = (float*)newmem->hostStorage()->data();

    // copy from old to new
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
		ShAttrib2f u = tc;
		if (m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT) {
			u = size()*frac(u/size());
		}
		ShAttrib1f scale = pow(0.5,floor(m_level)); // cooompte the reduction factor
		ShAttrib1f transition = frac(m_level);
		u *= scale; // reduce the size
		ShAttrib2f u2 = 0.5*u + size()*ShAttrib2f(ShAttrib1f(1.0),0.5*scale); // coordinates for the next level
		u = cond(m_level>=1.0, u+size()*ShAttrib2f(ShAttrib1f(1.0),scale), u);
		return (1.0-transition)*m_realtex[u] + transition*m_realtex[u2]; // do linear interpolation between the levels
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
		ShAttrib2f u = tc;
		if (m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT) {
			u = frac(u);
		}
		ShAttrib1f twothird(0.666666); // scale to the size of the original texture
		u(0) *= twothird;
		ShAttrib1f scale = pow(0.5,floor(m_level)); // compute the reduction factor
		ShAttrib1f transition = frac(m_level);
		u *= scale; // reduce the size
		ShAttrib2f u2 = 0.5*u + ShAttrib2f(twothird,0.5*scale); // coordinates for the next level
		u = cond(m_level>=1.0, u+ShAttrib2f(twothird,scale), u); // translate to the right position
		return (1.0-transition)*m_realtex(u) + transition*m_realtex(u2); // linear interpolation between the levels
	}

	// set the current layer to be rendered
	void setLevel(ShAttrib1f level) {
		m_level = level;
	}
	
private:
	void generateMipMap(float* texdata, int wstart, int hstart, int wend, int hend, int wnew, int hnew) {
		int stride = return_type::typesize;
		int width = (wend-wstart)/2;
		int height = (hend-hstart)/2;
		// do a linear interpolation to resample the image
		for(int x=0 ; x<width ; x++) {
			for(int y=0 ; y<height ; y++) {
				for(int e=0 ; e<stride ; e++) {
					texdata[((y+hnew)*m_realwidth + x+wnew)*stride + e] =	texdata[((2*y+hstart)*m_realwidth + 2*x+wstart)*stride + e] +
																																texdata[((2*y+hstart)*m_realwidth + 2*x+1+wstart)*stride +e] +
																																texdata[((2*y+1+hstart)*m_realwidth + 2*x+wstart)*stride +e] +
																																texdata[((2*y+1+hstart)*m_realwidth + 2*x+1+wstart)*stride +e];
					texdata[((y+hnew)*m_realwidth + x+wnew)*stride + e] /= 4;	
				}
			}
		}
		if(width > 1 && height > 1) // generate smaller image with the data just computed
			generateMipMap(texdata, wnew, hnew, wnew+width, hnew+height, wnew, height/2);
	}

  typename T::rectangular_type m_realtex; // the texture with the mipmap layers
	int m_realwidth;
	ShAttrib1f m_level; // current mipmap level
};


#endif
