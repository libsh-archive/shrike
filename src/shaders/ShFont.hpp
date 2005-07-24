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
#include <map>

using namespace SH;
//using namespace ShUtil;

// operation for comparing pairs
// it is used in map that has elements composed of two components
class Kernpair {
	public:
		Kernpair(int a, int b):p(a,b) { }

		bool operator < ( const Kernpair &other ) const {
			if( p.first < other.p.first )
				return true;
			else if( p.first > other.p.first)
				return false;
			else
				return (p.second < other.p.second);
		}

		int first() const {return p.first;} 
		int second() const {return p.second;} 
	private:
		std::pair<int, int> p;
};

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
  int smallgrid() const;     ///< Determine the size of the picture user wants
  int biggrid() const;     ///< Determine the size of the grid
  int maxgwidth() const; ///< Determine the max width of all glyphs
  int maxgheight() const; ///< Determine the max height of all glyphs
  int minhadvance() const; ///< Determine the min horizontal advance
  void ShFont::loadFont(const std::string&, int, int);
  void ShFont::stringEnd();
  void renderline(int gnum, const char * str, float mg, float ng);

  const float* coords(int) const;
  float* coords(int);
  
  ShMemoryPtr memory(int);
  ShPointer<const ShMemory> memory(int) const;
  
private:
  int m_glyphcount;
  int m_width, m_height;
  int m_elements;
  int m_smallgrid;
  int m_biggrid;
  int m_maxgwidth;
  int m_maxgheight;
  int m_minhadvance;
  int m_split;  // how many smallgrid in each biggrid
  float * sp;    // grids storing what glyph each small grid has
                // including x, y start position
  ShHostMemoryPtr *m_memory;
  std::map<int, int> hadvanceMap;
  std::map<int, int> yminMap;
  std::map<int, int> gheightMap;
  std::map<int, int> gwidthMap;
  std::map<int, int> offsetx;
  std::map<int, int> offsety;
  std::map<int, int> winoctree;
  std::map<int, int> hinoctree;

  std::map<Kernpair, int> kmap;
  bool getflag(float sx, float sy, float ex, float ey, int gw, int gh, int ox, int oy, int ow, int oh);
};

#endif
