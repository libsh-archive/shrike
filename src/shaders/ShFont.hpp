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
#ifndef SHFONT_HPP
#define SHFONT_HPP

#include <string>
#include <sh/sh.hpp>
/*
#include "sh/ShDllExport.hpp"
#include "ShRefCount.hpp"
#include "ShMemory.hpp"
*/

using namespace SH;
//using namespace ShUtil;

/** An image, consisting of a rectangle of floating-point elements.
 * This class makes it easy to read PNG files and the like from
 * files.   It stores the image data in a memory object which can
 * then be shared with array, table, and texture objects.
 */
class
SH_DLLEXPORT ShFont : public ShRefCountable {
public:
  ShFont(); ///< Construct an empty image
  //ShFont(int width, int height, int depth); ///< Construct a black
                                             ///image at the given width/height/depth
  //ShFont(const ShFont& other); ///< Copy an image

  ~ShFont();

  int glyphcount() const; ///< Determine the number of glyphs in the font
  int width() const; ///< Determine the width of the image for line segments
  int height() const; ///< Determine the height of the image
  int elements() const; ///< Determine the depth (floats per pixel) of
                         ///the image
  int psize() const;     ///< Determine the size of the picture user wants
  int gridsize() const;     ///< Determine the size of the grid
  int maxgwidth() const; ///< Determine the max height of all glyphs
  int maxgheight() const; ///< Determine the max height of all glyphs
  int minhadvance() const; ///< Determine the min horizontal advance
  void ShFont::loadFont(const std::string&);

  const float* coords(int) const;
  float* coords(int);
  
  ShMemoryPtr memory(int);
  ShPointer<const ShMemory> memory(int) const;
  
private:
  int m_glyphcount;
  int m_width, m_height;
  int m_elements;
  int m_psize;
  int m_gridsize;
  int m_maxgwidth;
  int m_maxgheight;
  int m_minhadvance;
  int m_split;  // how many psize in each gridsize
  ShHostMemoryPtr *m_memory;
  std::map<int, int> hadvanceMap;
  std::map<int, int> yminMap;
  std::map<int, int> gheightMap;
  std::map<int, int> gwidthMap;

  void texture(int, float *);
  void renderline(int gnum, int * str, float mg, float ng, float * sp);
};

#endif
