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
#ifndef LARGETEXTURE_HPP
#define LARGETEXTURE_HPP

#include <iostream>
#include <string>
#include <sh/sh.hpp>

using namespace SH;

template<typename T, int N, int M>
class LargeTexture : public T, public ShMemoryDep {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
	typedef typename T::base_type base_type;
	typedef LargeTexture<typename T::rectangular_type, N, M> rectangular_type;

	LargeTexture() : parent_type()
	{}

	LargeTexture(int width) : parent_type(width)
	{}

	LargeTexture(int width, int height) : parent_type(width, height)
	{}

	LargeTexture(int width, int height, int depth) : parent_type(width, height, depth)
	{}

  void memory(ShMemoryPtr mem) {
    this->m_node->memory(mem);
    this->m_node->memory()->add_dep(this);
    this->m_node->memory()->flush();
  }

  ShMemoryPtr memory() { return this->m_node->memory(); }

  void memory_update() {
    width = this->m_node->width() / N;
    height = this->m_node->height() / M;
    int stride = return_type::typesize;
    ShHostStoragePtr cursto =	shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
		float* olddata = (float*)cursto->data();
    for(int i=0 ; i<N ; i++) {
      for(int j=0 ; j<M ; j++) {
        ShHostMemoryPtr newtext = new ShHostMemory(width * height * stride * sizeof(float));
        textureTable[i][j].memory(newtext);
        textureTable[i][j].internal(true);
        textureTable[i][j].size(width, height);
   		  float* data = (float*)newtext->hostStorage()->data();
        for(int x=0 ; x<width ; x++) {
          for(int y=0 ; y<height ; y++) {
            for(int z=0 ; z<stride ; z++) {
              data[(y*width+x)*stride + z] = olddata[((y+j*height)*this->m_node->width()+x + i*width)*stride + z];
            }
          }
        }
      }
    }
	}
	
	return_type operator[](const ShTexCoord2f tc) const {
    ShAttrib2f pos = floor(tc / ShAttrib2f(width,height));
    ShAttrib2f u  = mad(-1.0, pos*ShAttrib2f(width,height), tc);
    return_type text;
    for(int i=0 ; i<N ; i++) {
      for(int j=0 ; j<M ; j++) {
        text = cond( SH::min(pos(0)>i-0.5, pos(1)>j-0.5), textureTable[i][j][u], text);
      }
    }
    return text;
	}
			
	return_type operator()(const ShTexCoord2f tc) const {
    return operator[](tc*this->size());
  }

private:
  T textureTable[N][M];
  int width, height;
};

#endif
