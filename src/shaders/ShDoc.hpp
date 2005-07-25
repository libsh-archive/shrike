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
  ShColor3f Alias(ShAttrib2f x,
  		      ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib2f m_thres);
  ShColor3f isoAntiOutline(ShAttrib2f x,
  	              ShColor3f m_color1, 
		      ShColor3f m_color2,
		      ShAttrib1f m_fw,
		      ShAttrib2f m_thres);
    /*
      case 3: {
        // isotropically antialiased outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 4: {
        // anisotropically antialiased outline rendering;
        ShAttrib2f fw;;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 5: {
        // gradient visualization
        o = 0.5 * (r(2) + 1.0) * m_vcolor1 
          + 0.5 * (r(3) + 1.0) * m_vcolor2;;
      } break;
      case 6: {
        // filter width visualization
        ShAttrib2f fw = fwidth(x);	      
        o = fw(0,1,0);
      } break;
      case 7: {
        // isotropically antialiased pseudodistance outline rendering
        ShAttrib2f fw = fwidth(x);;
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));;
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 8: {
        // anisotropically antialiased pseudodistance outline rendering
        ShAttrib2f fw;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;;
      case 9: {
        // biased signed distance map visualization 
        o = (0.5 + r(0) * m_scale)(0,0,0) 
          * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 10: {
        // biased signed distance map visualization 
        o = (0.5 + r(0) * m_scale)(0,0,0);
      } break;
      case 11: {
        // signed distance map visualization
        o = (abs(r(0)) * m_scale)(0,0,0) 
          * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 12: {
        // biased signed pseudodistance map visualization 
        o = (0.5 + r(1) * m_scale)(0,0,0) 
          * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 13: {
        // biased signed pseudodistance map visualization 
        o = (0.5 + r(1) * m_scale)(0,0,0);
      } break;
      default: {
        // pseudodistance visualization
        o = (abs(r(1)) * m_scale)(0,0,0) 
          * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
	       */

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
  ShAttrib2f size[4];
};

#endif
