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

#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "ShFont.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "dist_util.hpp"

class VectorDoc : public Shader {
public:
  VectorDoc(int mode);
  ~VectorDoc();

  bool init();
  ShAttrib4f sprite_dist(
  	const ShArrayRect<ShAttrib4f> & ftexture,
  	const ShArrayRect<ShAttrib1f> & flag,
  	const ShArrayRect<ShAttrib4f> & sprite1,
  	const ShArrayRect<ShAttrib4f> & sprite2,
        ShAttrib2f x,
        ShAttrib2f fx
  );

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:

  ShProgram vsh, fsh;
  
  int m_mode;
  static bool m_done_init;
  static ShAttrib1f m_size;
  static ShAttrib1f m_scale;
  static ShVector2f m_offset;
  static ShAttrib1f m_fw;

  static ShAttrib2f m_thres;
  static ShColor3f m_color1, m_color2;
  static ShColor3f m_vcolor1, m_vcolor2;

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

VectorDoc::VectorDoc(int mode)
  : Shader(std::string("Vector Graphics: Vector Document") + 
	  ((mode == 0) ? ": Isotropically Antialiased" : 
	  ((mode == 1) ? ": Anisotropically Antialiased" : 
	  ((mode == 2) ? ": Aliased" : 
	  ((mode == 3) ? ": Isotropically Antialiased Outline" : 
	  ((mode == 4) ? ": Anisotropically Antialiased Outline" : 
	  ((mode == 5) ? ": Gradient" : 
	  ((mode == 6) ? ": Filter Width" : 
	  ((mode == 7) ? ": Pseudodistance Isotropically Antialiased Outline" : 
	  ((mode == 8) ? ": Pseudodistance Anisotropically Antialiased Outline" : 
	  ((mode == 9) ? ": Biased Signed Distance" : 
	  ((mode == 10) ? ": Greyscale Biased Signed Distance" : 
	  ((mode == 11) ? ": Signed Distance" : 
	  ((mode == 12) ? ": Biased Signed Pseudodistance" : 
	  ((mode == 13) ? ": Greyscale Biased Signed Pseudodistance" : 
	                  ": Pseudodistance"))))))))))))))),
    m_mode(mode)
{
  if (!m_done_init) {
    // Initialize parameters and static variables
    m_scale.name("scale");
    m_scale.range(0.0, 30.0);

    m_fw.name("filter width");
    m_fw.range(0.0, 10.0);

    m_offset.name("offset");
    m_offset.range(-10.0, 10.0);

    m_size.name("size");
    m_size.range(0.0, 1000.0);

    m_thres.name("threshold");
    m_thres.range(-1.0, 1.0);

    m_color1.name("color1");
    m_color2.name("color2");
    m_vcolor1.name("vcolor1");
    m_vcolor2.name("vcolor2");

    m_done_init = true;
  }
}

VectorDoc::~VectorDoc()
{
}

ShAttrib4f
VectorDoc::sprite_dist (
    const ShArrayRect<ShAttrib4f> & ftexture,
    const ShArrayRect<ShAttrib1f> & flag,
    const ShArrayRect<ShAttrib4f> & sprite1,
    const ShArrayRect<ShAttrib4f> & sprite2,
    ShAttrib2f x,  // texture coordinates
    ShAttrib2f fx  // where to get sprite info
) {
    ShAttrib4f s1 = sprite1(fx);
    ShAttrib4f s2 = sprite2(fx);

    // s1(2,3) has the real width and height of the glyph
    // divided by maxheight get the ratio compared with max height
    s1(2,3) = s1(2,3) / maxgheight;

    // s2(0,1) has the offset of the glyph in octree texture
    // division get the start pos (0-1) of the glyph in octree texture
    s2(0,1) = ShAttrib2f(s2(0)/width, s2(1)/height);

    // s2(2,3) has the width, height of the glyph in octree texture
    // division get the percentage of the glyph size compared to the 
    // octree texture size
    ShAttrib2f scale2 = ShAttrib2f(width/s2(2), height/s2(3));

    // convert coords from texture domean to octree domain
    // s1(0,1) has the exact coords of the glyph in the texture
    // tx1: if we treat the real size of glyph as from 0-1,
    //      it gets the coords of x in terms of the glyph size
    // *biggrid = smallgrid / m_split
    ShAttrib2f tx1 = (x - s1(0,1)) * biggrid / s1(2,3);
    tx1 = clamp(tx1,0.0,1.0);
    
    // get the corresponding coords of x in octree texture
    ShAttrib2f tx2 = tx1 / scale2 + s2(0,1);

    ShAttrib4f L[4];

    // there are 4 edges with each one stored at each corner of 
    // the cell that x falls in.  We need to compute all of the them.  
    // And then get the shortest one.
    for(int i=0; i<4; i++) {
	ShAttrib2f y = tx2 + size[i];
    	L[i] = ftexture(y); 
	L[i] = L[i] * s1(2,3,2,3) / biggrid + s1(0,1,0,1);
    }
  
    ShAttrib4f r = segdists_a(L,4,x);

    // mask off the entirely inside and entirely outside cells
    // (this lets us reuse their storage for adjacent boundary cells)
    r(0,1) += 1.0e13*flag(tx2 + size[0]);

    return r;
}

bool VectorDoc::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  font.loadFont("font.txt", 256, 16);

  // get info about general font, sprite
  // and the octree texture
  glyphcount = font.glyphcount();
  width = font.width();
  height = font.height();
  smallgrid = font.smallgrid();
  biggrid = font.biggrid();
  maxgwidth = font.maxgwidth();
  maxgheight = font.maxgheight();
  elements = 4;

  std::cerr << "in VectorDoc file: width " << width << " height " << height << " glyphcount ";
  std::cerr << glyphcount << std::endl;

  // octree textures for line segment endpoints and flags
  // which indicate if a cell is totally inside or outside
  // of a glyph
  ShArrayRect<ShAttrib4f> ftexture(width, height);
  ShArrayRect<ShAttrib1f> flag(width, height);

  // textures for sprites
  ShArrayRect<ShAttrib4f> sprite1(smallgrid, smallgrid);
  ShArrayRect<ShAttrib4f> sprite2(smallgrid, smallgrid);

  // copy info from class font to the array
  // ftexture: octree texture for line segment endpoints
  // flag:     octree texture for flags
  // sprite1:  locations of sprites and their start x, y pos
  // sprite2:  offset and size of glyphs in the octree texture
  ftexture.memory(font.memory(1));
  flag.memory(font.memory(2));
  sprite1.memory(font.memory(3));
  sprite2.memory(font.memory(4));

  size[0] = ShAttrib2f(-0.5/width, -0.5/height);
  size[1] = ShAttrib2f( 0.5/width, -0.5/height);
  size[2] = ShAttrib2f(-0.5/width,  0.5/height);
  size[3] = ShAttrib2f( 0.5/width,  0.5/height);

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = m_size*tc - m_offset;

    ShAttrib4f sprite_r[4];
    sprite_r[0] = sprite_dist(
	// these should be class member variables
    	ftexture, flag, sprite1, sprite2, 
	// these should be parameters
	x,
	x
    );
    sprite_r[1] = sprite_dist(
	// these should be class member variables
    	ftexture, flag, sprite1, sprite2, 
	// these should be parameters
	x,
	x - ShAttrib2f(1.0/smallgrid,0.0)
    );

    ShAttrib4f r = cond(sprite_r[0](0) < sprite_r[1](0),sprite_r[0],sprite_r[1]);
    r(2,3) = normalize(r(2,3));

    switch (m_mode) {
      case 0: {
        // isotropically antialiased rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 1: {
        // anisotropically antialiased rendering
        ShAttrib2f fw;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 2: {
        // aliased rendering;
        o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
      } break;
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
    }
  } SH_END_PROGRAM;
  return true;
}

bool VectorDoc::m_done_init = false;
ShAttrib1f VectorDoc::m_scale = ShAttrib1f(1.0);
ShVector2f VectorDoc::m_offset = ShVector2f(0,0);
ShAttrib1f VectorDoc::m_size = ShAttrib1f(1.0);
ShAttrib1f VectorDoc::m_fw = ShAttrib1f(1.0);
ShAttrib2f VectorDoc::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f VectorDoc::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorDoc::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorDoc::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f VectorDoc::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

VectorDoc vd_iaa = VectorDoc(0);
VectorDoc vd_aaa = VectorDoc(1);
VectorDoc vd_naa = VectorDoc(2);
VectorDoc vd_iaa_outline = VectorDoc(3);
VectorDoc vd_aaa_outline = VectorDoc(4);
VectorDoc vd_grad = VectorDoc(5);
VectorDoc vd_fw = VectorDoc(6);
VectorDoc vd_pd_iaa_outline = VectorDoc(7);
VectorDoc vd_pd_aaa_outline = VectorDoc(8);
VectorDoc vd_biased_distance = VectorDoc(9);
VectorDoc vd_grey_biased_distance = VectorDoc(10);
VectorDoc vd_distance = VectorDoc(11);
VectorDoc vd_biased_pdistance = VectorDoc(12);
VectorDoc vd_grey_biased_pdistance = VectorDoc(13);
VectorDoc vd_pdistance = VectorDoc(14);

