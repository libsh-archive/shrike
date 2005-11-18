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

#ifndef SHDOC_HPP
#define SHDOC_HPP

#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include "ShFont.hpp"

using namespace SH;
using namespace ShUtil;

#include "dist_util.hpp"

class ShDoc{
public:
  ShDoc();
  ~ShDoc();

  bool initFont(std::string, int, int);
  void string(int gnum, const int * gly, float mg, float ng);
  void string(int gnum, const char * str, float mg, float ng);
  void stringEnd();
  ShAttrib4f shortestDis(ShAttrib2f x);
  ShColor3f isoAntialias(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f anisoAntialias(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f ShDoc::anisoAntialiasPhong(
		      ShNormal3f nv,
		      ShVector3f hv,
		      ShVector3f lv,
		      ShPoint3f pv,
		      ShAttrib1f phongexp,
		      ShAttrib3f th,
	              ShAttrib2f x,
	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f ShDoc::anisoAntialiasPhongEmboss(
		      ShNormal3f nv,
		      ShVector3f hv,
		      ShVector3f lv,
		      ShPoint3f pv,
		      ShVector3f tgt0,
		      ShVector3f tgt1,
		      ShAttrib1f phongexp,
		      ShAttrib3f th,
	              ShAttrib2f x,
	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f Alias(ShAttrib2f x,
  		      ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib2f m_thres);
  ShColor3f isoAntiOutline(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f anisoAntiOutline(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f gradient(ShAttrib2f x,
		       ShColor3f m_vcolor1, 
		       ShColor3f m_vcolor2);
  ShColor3f filterWidth(ShAttrib2f x);
  ShColor3f isoAntiPseudoOutline(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f anisoAntiPseudoOutline(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
  ShColor3f biasSignedDis1(ShAttrib2f x,
		      ShAttrib1f m_scale,
		      ShColor3f m_vcolor1,
		      ShColor3f m_vcolor2);
  ShColor3f biasSignedDis2(ShAttrib2f x,
		      ShAttrib1f m_scale);
  ShColor3f signedDisMap(ShAttrib2f x,
	              ShAttrib1f m_scale,
		      ShColor3f m_vcolor1,
		      ShColor3f m_vcolor2);
  ShColor3f biasSignPseudoMap(ShAttrib2f x,
	              ShAttrib1f m_scale,
		      ShColor3f m_vcolor1,
		      ShColor3f m_vcolor2);
  ShColor3f biasSignPserdoMap(ShAttrib2f x,
	              ShAttrib1f m_scale);
  ShColor3f ShDoc::pseudoDis(ShAttrib2f x,
                      ShAttrib1f m_scale,
	    	      ShColor3f m_vcolor1,
		      ShColor3f m_vcolor2);

private:

  ShAttrib4f sprite_dist(
        ShAttrib2f x,
        ShAttrib2f fx
  );

  // parameters for the interface
  static ShAttrib1f m_size;
  static ShVector2f m_offset;
  //static ShAttrib1f m_scale;
  //static ShAttrib1f m_fw;

  //static ShAttrib2f m_thres;
  //static ShColor3f m_color1, m_color2;
  //static ShColor3f m_vcolor1, m_vcolor2;

  // parameters for own use

  // octree textures for line segment endpoints and flags
  // which indicate if a cell is totally inside or outside
  // of a glyph
  ShArrayRect<ShAttrib4f> ftexture;
  ShArrayRect<ShAttrib1f> flag;

  // textures for sprites
  ShArrayRect<ShAttrib4f> sprite1;
  ShArrayRect<ShAttrib4f> sprite2;

  ShFont font;
  int glyphcount;
  int width;
  int height;
  int smallgrid;
  int biggrid;
  int maxgwidth;
  int maxgheight;
  int elements;
  ShAttrib2h size[4];
};

#endif
