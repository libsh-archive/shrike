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
#ifndef CLIPMAP_HPP
#define CLIPMAP_HPP

#include <iostream>
#include <math.h>
#include <string>
#include <sh/sh.hpp>

using namespace SH;
using namespace std;

template<typename T, int ClipSize>
class ClipMap : public T, public ShMemoryDep {
public:
  typedef T parent_type;
  typedef typename T::return_type return_type;
  typedef typename T::base_type base_type;
  typedef ClipMap<typename T::rectangular_type, ClipSize> rectangular_type;

  ClipMap() : parent_type() {
    init = false;
  }

  ClipMap(int width) : parent_type(width) {
    init = false;
  }
  
  ClipMap(int width, int height) : parent_type(width, height) {
    init = false;
  }

  ClipMap(int width, int height, int depth) : parent_type(width, height, depth) {
    init = false;
  }

  void setCenter(int center[2]) {
    NewCenter[0] = center[0];
    NewCenter[1] = center[1];
    m_node->memory()->flush();
  }    

  void memory(ShMemoryPtr mem) {
    m_node->memory(mem);
    m_node->memory()->add_dep(this);
  }

  ShMemoryPtr memory() {
    return m_node->memory();
  }

  void memory_update() {
    if(!init) {
      initClipMap();
      init = true;
    }
    else {
      clipmapmem->hostStorage()->dirty();
      int difference[2];
      difference[0] = NewCenter[0] - oldcenter[0];
      difference[1] = NewCenter[1] - oldcenter[1];
      // if the direction has changed or the difference is too big, recompute all the stack images
      int dir = difference[0]*olddiff[0] + difference[1]*olddiff[1];
      if(abs(NewCenter[0] - origin[0]) >= ClipSize || abs(NewCenter[1] - origin[1]) >= ClipSize || dir < 0) {
        initStack();
      }
      else { // update just a small part
        // update the horizontal changement
        horizontalUpdate(difference, offset);
        // the vertical changement
        verticalUpdate(difference, offset);
        // the diagonal changement, need to make the difference with the center of the original image
        difference[0] = NewCenter[0] - origin[0];
        difference[1] = NewCenter[1] - origin[1];
        diagonalUpdate(difference);
        // save the new parameters
        offset[0] += NewCenter[0] - oldcenter[0];
        offset[1] += NewCenter[1] - oldcenter[1];
        oldcenter[0] = NewCenter[0];
        oldcenter[1] = NewCenter[1];
        olddiff[0] = difference[0];
        olddiff[1] = difference[1];
      }
    }
  }
  
  void initStack()
  {
    // new stack = new origin
    oldcenter[0] = NewCenter[0];
    oldcenter[1] = NewCenter[1];
    origin[0] = oldcenter[0];
    origin[1] = oldcenter[1];
    olddiff[0] = 0;
    olddiff[1] = 0;
    // copy the whole window
    int updateWindow[2];
    updateWindow[0] = ClipSize;
    updateWindow[1] = ClipSize;
    // the upper left point of the data to copy
    int center[2];
    center[0] = NewCenter[0] - ClipSize/2;
    center[1] = NewCenter[1] - ClipSize/2;
    // the upper left point on the texture
    offset[0] = 0;
    offset[1] = 0;
    // level 0, high details, no rescaling
    int mipmaplevel = 0;
    copyClipMapData(updateWindow, offset, center, mipmaplevel);
    
    // next levels of the stacks
    int s = 1;
    while(s < stackSize) {
      // change the mipmap level
      mipmaplevel++; 
      // compute the new center
      float factor = pow(2.0,mipmaplevel-1);
      center[0] -= (int)(factor * ClipSize/2);
      center[1] -= (int)(factor * ClipSize/2);
      // compute the new position on the texture
      offset[0] += ClipSize;
      // copy the data
      copyClipMapData(updateWindow, offset, center, mipmaplevel);
      // inc counter
      s++;
    }
    // save the null offset
    offset[0] = 0;
    offset[1] = 0;
  }
  
  void initClipMap()
  {
    stride = return_type::typesize; // save the number of elements
    generateMipMap(); // generate the mipmap levels of the original texture
    int scalemax = m_node->width() > m_node->height() ? m_node->width() : m_node->height();
    stackSize = (int)ceil(log((float)scalemax/ClipSize) / 0.693147181); // get the number of levels that will be clamped
    float scale = 1.5 * pow(0.5,stackSize);
    width = stackSize*ClipSize + (int)(m_node->width()*scale); // the new width
    height = ClipSize; // the new height
    // allocate the memory space for the texture
    clipmapmem = new ShHostMemory(width * height * stride * sizeof(float));
    m_clipmap.memory(clipmapmem);
    m_clipmap.size(width, height); // set the size
    
    initStack(); // fill the stack levels
    // fill the pyramid levels
    // done only once as all the image is saved and no change should be applied after that
    int center[2];
    center[0] = 0;
    center[1] = 0;
    // set the position on the clipmap texture
    int coffset[2];
    coffset[0] = stackSize*ClipSize;
    coffset[1] = 0;
    // all the image is copied
    int updateWindow[2];
    updateWindow[0] = (int)(pow(0.5, stackSize) * m_node->width());
    updateWindow[1] = (int)(pow(0.5, stackSize) * m_node->height());
    // copy data
    int mipmaplevel = stackSize;
    copyClipMapData(updateWindow, coffset, center, mipmaplevel);
    coffset[0] += (int)(pow(0.5,stackSize) * m_node->width());
    coffset[1] = height;
    do { // copy the other levels until the end of the mipmap levels
      coffset[1]/=2;
      if(updateWindow[0] > 1 && updateWindow[1] > 1) {
        updateWindow[0]/=2;
        updateWindow[1]/=2;
        mipmaplevel++;
      }
      copyClipMapData(updateWindow, coffset, center, mipmaplevel);
    } while(coffset[1] > 0);
  }

  return_type operator[](const ShTexCoord2f tc) const {
    ShAttrib2f center(oldcenter[0], oldcenter[1]);
    ShAttrib2f offset(offset[0], offset[1]);
    
    ShAttrib1f dist = SH::max(abs(tc-center)+1.0);
    ShAttrib1f level = pos(log2(dist/(0.5*ClipSize))+1.0); // get clip zones
 	  level += pos(log2(SH::max(fwidth(tc)))); // add mip-mapping
    
    ShAttrib1f transition = frac(level); // blending coefficient
    level = floor(level); // base coefficient
    
    ShAttrib1f factor = pow(2.0,level);
    ShAttrib1f scale = rcp(factor);
    ShAttrib2f u = scale * tc; // the texture coordinates on the real texture
    ShAttrib2f u2 = 0.5*u;
    
    ShAttrib2f stackTranslation(ClipSize * level, (ShAttrib1f)0.0);
    ShAttrib2f pyramidTranslation((ShAttrib1f)ClipSize*stackSize  + pow(0.5,stackSize)*m_node->width(), ClipSize*pow(0.5,pos(level-stackSize)));
    // wrap arround the center point
    u = cond(level<stackSize, frac((u-scale*(center-offset-factor*0.5*ClipSize))/ClipSize)*ClipSize, u);
    ShAttrib2f translation = cond(level>stackSize, pyramidTranslation, stackTranslation);
    u += translation; // go to the correct position on the texture
    // need 2nd texture coordinates for blending, same alorithm used
    level = level + 1.0;
    u2 = cond(level<stackSize, frac((u2-0.5*scale*(center-offset-factor*ClipSize))/ClipSize)*ClipSize, u2);
    translation = cond(level>stackSize, pyramidTranslation, stackTranslation + ShAttrib2f((float)ClipSize,0.0));
    translation(1) /= 2;
    u2 += translation;
    return (1.0-transition)*m_clipmap[u] + transition*m_clipmap[u2]; // blend
  }

  return_type operator()(const ShTexCoord2f tc) const {
    return operator[](tc*size());
  }
  
  
private:
  // generate a mipmap texture
  void ClipMap::generateMipMap() {
    width = m_node->width();
    height = m_node->height();
    oldwidth = width;
    oldheight = height;
    mipmapwidth = width + width/2 + width%2;
    ShHostStoragePtr cursto = shref_dynamic_cast<ShHostStorage>(memory()->findStorage("host"));
    float* olddata = (float*)cursto->data();
    mipmapmem = new ShHostMemory(mipmapwidth * height * stride * sizeof(float));
    float* data = (float*)mipmapmem->hostStorage()->data();
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        for (int e = 0; e < stride; e++) {
          data[(y*mipmapwidth + x)*stride + e] =  olddata[(y*width + x)*stride + e];
        }
      }
    }
    generateMipMapLevels(data, 0, 0, width, height, width, height/2); // add mipmap levels by recursion
  }

  // recursive computation of the levels 
  void ClipMap::generateMipMapLevels(float* data, int wstart, int hstart, int wend, int hend, int wnew, int hnew) {
    width = (wend-wstart)/2;
    height = (hend-hstart)/2;
    // do a linear interpolation to resample the image
    for(int x=0 ; x<width ; x++) {
      for(int y=0 ; y<height ; y++) {
        for(int e=0 ; e<stride ; e++) {
          int maxx = 2*x+1 < 2*width ? 2*x+1 : 2*width-1;
          int maxy = 2*y+1 < 2*height ? 2*y+1 : 2*height-1;
          data[((y+hnew)*mipmapwidth + x+wnew)*stride + e] =  data[((2*y+hstart)*mipmapwidth + 2*x+wstart)*stride +e] +
                                                              data[((2*y+hstart)*mipmapwidth + maxx+wstart)*stride +e] +
                                                              data[((maxy+hstart)*mipmapwidth + 2*x+wstart)*stride +e] +
                                                              data[((maxy+hstart)*mipmapwidth + maxx+wstart)*stride +e];
          data[((y+hnew)*mipmapwidth + x+wnew)*stride + e] *= 0.25; 
        }
      }
    }
    if(width > 1 && height > 1) // generate smaller image with the data just computed
      generateMipMapLevels(data, wnew, hnew, wnew+width, hnew+height, wnew, height/2);
  }
  
  // copy a part of a mipmap level into a clipmap level
  void copyClipMapData(int updateWindow[2], int offset[2], int center[2], int mipmaplevel) {
    // compute the size of the current mipmap level
    int mipmapsize[2];
    float scalefactor = pow(0.5, (double)mipmaplevel);
    mipmapsize[0] = (int)(m_node->width() * scalefactor);
    mipmapsize[1] = (int)(m_node->height() * scalefactor);
    // compute the center coordinates at the current mipmap level
    int mipmapcenter[2];
    if(mipmaplevel>0) { // need to scale and translate to the right part
      mipmapcenter[0] = (int)(center[0] * scalefactor) + m_node->width();
      mipmapcenter[1] = (int)(center[1] * scalefactor) + mipmapsize[1];
    }
    else { // 1st level, no change
      mipmapcenter[0] = center[0];
      mipmapcenter[1] = center[1];
    }
    // copy the data
    float* mipmapdata = (float*)mipmapmem->hostStorage()->data();
    float* data = (float*)clipmapmem->hostStorage()->data(); 
    for(int i=0 ; i<updateWindow[0] ; i++) {
      for(int j=0 ; j<updateWindow[1] ; j++) {
        for(int k=0 ; k<stride ; k++) {
          // wrapping
          int x = mipmapcenter[0]+i;
          int y = mipmapcenter[1]+j;
          if(mipmaplevel == 0) {
            if(x<0)
              x += m_node->width();
            if(y<0)
              y += m_node->height();
            if(x>m_node->width())
              x -= m_node->width();
            if(y>m_node->height())
              y -= m_node->height();
          }
          else {
            if(x<m_node->width())
              x += mipmapsize[0];
            if(x>m_node->width()+mipmapsize[0])
              x -= mipmapsize[0];
            if(y>2*mipmapsize[1])
              y -= mipmapsize[1];
            if(y<mipmapsize[1])
              y += mipmapsize[1];
          }
          // copy the data
          data[((j+offset[1])*width + i+offset[0])*stride + k] =
            mipmapdata[(y*mipmapwidth + x)*stride + k];
        }
      }
    }
  }

  void horizontalUpdate(int difference[2], int offset[2]) {
    int updateWindow[2], coffset[2], center[2];
    updateWindow[0] = abs(difference[0]);
    updateWindow[1] = ClipSize - abs(difference[1]) - abs(offset[1]);
    coffset[0] = offset[0];
    coffset[1] = offset[1];
    center[0] = NewCenter[0];
    center[1] = NewCenter[1];

    if(difference[0] >= 0) {
      center[0] = oldcenter[0] + ClipSize/2;
    }
    else {
      center[0] = center[0] - ClipSize/2;
      coffset[0] += difference[0];
      if(coffset[0] < 0)
       coffset[0] += ClipSize;
    }
    if(difference[1] >= 0) {
      center[1] = oldcenter[1] - ClipSize/2 + difference[1];
      coffset[1] += difference[1];
      if(coffset[1] < 0)
        coffset[1] += ClipSize;
    }
    else {
      center[1] = origin[1] - ClipSize/2;
      coffset[1] = 0;
    }
    int mipmaplevel = 0;
    copyClipMapData(updateWindow, coffset, center, mipmaplevel);
      
    int s = 1;
    while(s<stackSize) {
      // change the mipmap level
      mipmaplevel++; 
      // scale the window
      updateWindow[0] /= 2;
      float factor = pow(0.5f,(float)mipmaplevel);
      updateWindow[1] += (int)(factor * (abs(difference[1]+offset[1]))); 
      // compute the new center
      if(difference[0] > 0)
        center[0] += mipmaplevel * ClipSize/2;
      else {
        center[0] -= mipmaplevel * ClipSize/2;
        coffset[0] += updateWindow[0];
      }
      center[1] -= mipmaplevel * (ClipSize/2);
      if(difference[1] >= 0) {
        coffset[1] -= (int)(factor*abs(difference[1])) + (int)(factor*offset[1]);
      }
      coffset[0] += ClipSize - (int)(factor*offset[0]);
      // copy the data
      copyClipMapData(updateWindow, coffset, center, mipmaplevel);
      s++; // inc counter
    }
  }

  void verticalUpdate(int difference[2], int offset[2]) {
    int updateWindow[2], coffset[2], center[2];
    coffset[0] = offset[0];
    coffset[1] = offset[1];
    updateWindow[0] = ClipSize - abs(difference[0]) - abs(offset[0]);
    updateWindow[1] = abs(difference[1]);
    center[0] = NewCenter[0];
    center[1] = NewCenter[1];
    
    if(difference[1] >= 0) {
      center[1] = oldcenter[1] + ClipSize/2;
    }
    else {
      center[1] = center[1] - ClipSize/2;
      coffset[1] = offset[1] + difference[1];
      if(coffset[1] < 0)
        coffset[1] += ClipSize;
    }
    if(difference[0] >= 0) {
      center[0] = oldcenter[0] - ClipSize/2 + difference[0];
      coffset[0] += difference[0];
      if(coffset[0] < 0)
        coffset[0] += ClipSize;
    }
    else {
      center[0] = origin[0] - ClipSize/2;
      coffset[0] = 0;
    }
    int mipmaplevel = 0;
    copyClipMapData(updateWindow, coffset, center, mipmaplevel);
      
    int s = 1;
    while(s<stackSize) {
      // change the mipmap level
      mipmaplevel++; 
      updateWindow[1] /= 2;
      float factor = pow(0.5f,(float)mipmaplevel);
      updateWindow[0] += (int)(factor * (abs(difference[0]+offset[0]))); 
      // compute the new center
      if(difference[1] > 0) {
        center[1] += mipmaplevel * ClipSize/2;
        coffset[1] /= 2;
      }
      else {
        center[1] -= mipmaplevel * ClipSize/2;
        coffset[1] += updateWindow[1] + (int)(factor*abs(offset[1]));
      }
      center[0] -= mipmaplevel * (ClipSize/2);
      coffset[0] += ClipSize;
      if(difference[0] >= 0)
        coffset[0] -= (int)(factor * (abs(difference[0]) + offset[0]));
      // copy the data
      copyClipMapData(updateWindow, coffset, center, mipmaplevel);
      s++;
    }
  }

  void diagonalUpdate(int difference[2]) {
    int updateWindow[2], coffset[2], center[2];
    updateWindow[0] = abs(difference[0]);
    updateWindow[1] = abs(difference[1]);
    center[0] = NewCenter[0];
    center[1] = NewCenter[1];
    
    if(difference[0] > 0) {
      center[0] = origin[0] + ClipSize/2;
      coffset[0] = 0;
    }
    else {
      center[0] -= ClipSize/2;
      coffset[0] = ClipSize + difference[0];
    }
    if(difference[1] > 0) {
      center[1] = origin[1] + ClipSize/2;
      coffset[1] = 0;
    }
    else {
      center[1] -= ClipSize/2;
      coffset[1] = ClipSize + difference[1];
    }
    int mipmaplevel = 0;
    copyClipMapData(updateWindow, coffset, center, mipmaplevel);
    int s = 1;
    while(s<stackSize) {
      // change the mipmap level
      mipmaplevel++; 
      updateWindow[0] /= 2; 
      updateWindow[1] /= 2;
      // compute the new center
      if(difference[0] > 0)
        center[0] += mipmaplevel * ClipSize/2;
      else {
        center[0] -= mipmaplevel * ClipSize/2;
        coffset[0] += updateWindow[0];
      }
      if(difference[1] > 0)
        center[1] += mipmaplevel * ClipSize/2;
      else {
        center[1] -= mipmaplevel * ClipSize/2;
        coffset[1] += updateWindow[1];
      }
      coffset[0] += ClipSize;
      // copy the data
      copyClipMapData(updateWindow, coffset, center, mipmaplevel);
      s++;
    }
  }

  typename T::rectangular_type m_clipmap;
  ShHostMemoryPtr clipmapmem, mipmapmem;
  int oldcenter[2];
  int origin[2];
  int offset[2];
  int olddiff[2];
  int NewCenter[2];
  int width, height, stride;
  int oldwidth, oldheight;
  int stackSize;
  int mipmapwidth;
  bool init;
};

#endif
