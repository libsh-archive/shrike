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
#ifndef SH_SHOBJFILE_HPP
#define SH_SHOBJFILE_HPP

#include <iosfwd>
#include <vector>
#include <sh/sh.hpp>

namespace SH {

struct ShObjFace {
  int points[3];
  int texcoords[3];
  int normals[3];
  int tangents[3];
};
  
class ShObjFile {
public:
  ShObjFile();
  ShObjFile(const ShObjFile& other);
  ~ShObjFile();

  ShObjFile& operator=(const ShObjFile& other);
  
  std::vector<ShPoint3f> vertices;
  std::vector<ShTexCoord2f> texcoords;
  std::vector<ShNormal3f> normals;
  std::vector<ShVector3f> tangents;
  std::vector<ShObjFace> faces;

private:

  friend std::istream& operator>>(std::istream& in, ShObjFile& obj);
};

}

#endif
