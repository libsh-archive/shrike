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
class MipMap : public T, public ShMemoryDep {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
  typedef typename T::base_type base_type;
  typedef MipMap<typename T::rectangular_type> rectangular_type;

  MipMap() : parent_type() {
    this->m_node->traits().filtering(ShTextureTraits::SH_FILTER_MIPMAP);
  }
  MipMap(int width) : parent_type(width) {
    this->m_node->traits().filtering(ShTextureTraits::SH_FILTER_MIPMAP);
  }
  MipMap(int width, int height) : parent_type(width, height) {
    this->m_node->traits().filtering(ShTextureTraits::SH_FILTER_MIPMAP);
  }
  MipMap(int width, int height, int depth) : parent_type(width, height, depth) {
    this->m_node->traits().filtering(ShTextureTraits::SH_FILTER_MIPMAP);
  }

  void memory(ShMemoryPtr mem) 
  {
    this->m_node->memory(mem); // set the data
    this->m_node->memory()->add_dep(this); // add the dependency to the update function
    this->m_node->memory()->flush(); // update the data
  }

  ShMemoryPtr memory() 
  {
    return this->m_node->memory();
  }

  /* test if the current texture has to used the new operations defined
   * or if it is fully supported by the hardware
   */
  bool test_type() 
  {
  // TODO: figure if this function should be moved in some backend test...
    if(this->m_node->traits().clamping() == ShTextureTraits::SH_UNCLAMPED)
      return true;
    return false;
  }

  // create a new texture memory
  // save the original texture and all the mipmap levels on its right
  void memory_update() 
  {
    int width = this->m_node->width();
    int height = this->m_node->height();
    int stride = return_type::typesize;
    // access the original data
    ShHostStoragePtr cursto = shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
    float* olddata = (float*)cursto->data();
    // create a new memory for the new texture
    m_mipmapwidth = width + width/2 + (width % 2);
    ShHostMemoryPtr newmem = new ShHostMemory(m_mipmapwidth*height*stride*sizeof(float), SH_FLOAT);
    float* newdata = (float*)newmem->hostStorage()->data();
    // update the new texture parameters
    m_mipmaptex.memory(newmem);
    m_mipmaptex.size(m_mipmapwidth, this->m_node->height());

    // copy the original data to the level 0
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        for (int e = 0; e < stride; e++) {
          newdata[(y*m_mipmapwidth + x)*stride + e] =
                    olddata[(y*width + x)*stride + e];
        }
      }
    }
    // create the low resolution levels
    int center[2] = {0,0};
    int offset[2] = {width,height/2};
    do {
      // shrink the image
      width /= 2;
      height /= 2;
      // bilinear interpolation on the data
      for(int x=0 ; x<width ; x++) {
        for(int y=0 ; y<height ; y++) {
          for(int e=0 ; e<stride ; e++) {
            // compute the limits of the coordinates
            int maxx = 2*x+1 < 2*width ? 2*x+1 : 2*width-1;
            int maxy = 2*y+1 < 2*height ? 2*y+1 : 2*height-1;
            newdata[((y+offset[1])*m_mipmapwidth + x+offset[0])*stride + e] =
              0.25*(newdata[((2*y+center[1])*m_mipmapwidth + 2*x+center[0])*stride + e] +
                    newdata[((2*y+center[1])*m_mipmapwidth + maxx+center[0])*stride +e] +
                    newdata[((maxy+center[1])*m_mipmapwidth + 2*x+center[0])*stride +e] +
                    newdata[((maxy+center[1])*m_mipmapwidth + maxx+center[0])*stride +e]);
          }
        }
      }
      // update the position for the next level
      center[0] = this->m_node->width();
      center[1] = offset[1];
      offset[1] /= 2;
    } while (width > 0 && height > 0);
  }
  
  return_type operator[](const ShTexCoord2f tc) const 
  {
    ShAttrib2f u = fwidth(tc); // derivatives of the tex coordinates
    ShAttrib1f level = pos(log2(max(u))); // mip-map level
    ShAttrib1f scale = pow(0.5,floor(level)); // reduction factor
    ShAttrib1f transition = frac(level); // blending factor
    if(this->m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT)
      u = scale*frac(tc/this->size())*this->size();
    else
      u = scale*tc; // size of the current level
    // compute the coordinates ot the next level
    ShAttrib2f u2 = 0.5*u + this->size()*ShAttrib2f(ShAttrib1f(1.0),0.5*scale);
    // translate the coordinates when needed
    u = cond(level<1.0, u, u+this->size()*ShAttrib2f(ShAttrib1f(1.0),scale));
    // return the linear interpolation of the two levels
    return (1.0-transition)*m_mipmaptex[u] +
            transition*m_mipmaptex[u2];
  }
      
  return_type operator()(const ShTexCoord2f tc) const 
  {
    ShAttrib2f u = fwidth(tc)*this->size();// derivatives of the tex coordinates
    ShAttrib1f level = pos(log2(max(u))); // mip-map level
    ShAttrib1f scale = pow(0.5,floor(level)); // reduction factor
    ShAttrib1f transition = frac(level); // blending factor
    if(this->m_node->traits().wrapping() == ShTextureTraits::SH_WRAP_REPEAT)
      u = scale*frac(tc);
    else
      u = scale*tc;
    u(0) *= 0.6666; // size of the original data
    // compute the coordinates ot the next level
    ShAttrib2f u2 = 0.5*u + ShAttrib2f(ShAttrib1f(0.6666),0.5*scale);
    // translate the coordinates when needed
    u = cond(level<1.0, u, u+ShAttrib2f(ShAttrib1f(0.6666),scale));
    // return the linear interpolation of the two levels
    return (1.0-transition)*m_mipmaptex(u) +
            transition*m_mipmaptex(u2);
  }

private:
  typename T::rectangular_type m_mipmaptex; // the texture with the mipmap layers
  int m_mipmapwidth; // the width of the rectangular texture

};


#endif
