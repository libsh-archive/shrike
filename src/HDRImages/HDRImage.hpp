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
#ifndef HDR_IMAGE_H
#define HDR_IMAGE_H

#include <string>
#include <sh/sh.hpp>

class HDRImage {
public:
  HDRImage();
  HDRImage(int width, int height, int depth);
  HDRImage(const HDRImage& other);
  ~HDRImage();
  
  HDRImage& operator=(const HDRImage& other);
  
  /*
    void loadEXR(const char filename[]); // load an OpenEXR file
    void saveEXR(const char filename[]); // save an OpenEXR file
  */
  
  void loadHDR(const char filename[]); // load a HDR file
  void saveHDR(const char filename[]); // save a HDR file
  
  float operator()(int x, int y, int i) const { return m_image(x,y,i); }
  float& operator()(int x, int y, int i) { return m_image(x,y,i); }
  
  int width() const { return m_width; }
  int height() const { return m_height; }
  int elements() const { return m_elements; }
  
  void loadPng(const std::string& filename) { m_image.loadPng(filename); }
  
  void savePng(const std::string& filename) { m_image.savePng(filename); }  
  
  void savePng16(const std::string& filename) { m_image.savePng16(filename); }
  
  SH::ShImage getNormalImage() { return m_image.getNormalImage(); }
  
  const float* data() const { return m_image.data(); }
  float* data() { return m_image.data(); }
	
  void dirty() { m_image.dirty(); }
  SH::ShMemoryPtr memory() { return m_image.memory(); }
  SH::ShPointer<const SH::ShMemory> memory() const { return m_image.memory(); }
  
  SH::ShImage image() { return m_image; }
  
private:
  SH::ShImage m_image;
  int m_width, m_height;
  int m_elements;
};

#endif
