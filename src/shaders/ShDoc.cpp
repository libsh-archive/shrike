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
#include "ShDoc.hpp"
#include "dist_util.hpp"

using namespace SH;
using namespace ShUtil;

ShDoc::ShDoc()
{
}

ShDoc::~ShDoc()
{
}

// =============================================================
// find out the shortest distance of a point to edges
// the four edges are stored at the 4 corners of each cell
// =============================================================
ShAttrib4f
ShDoc::sprite_dist (
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
    //      it gets the percent of x in terms of the glyph size
    // *biggrid = smallgrid / m_split
    ShAttrib2f tx1 = (x - s1(0,1)) * biggrid / s1(2,3);
    tx1 = clamp(tx1,0.0,1.0);
    
    // get the corresponding coords of x in octree texture
    ShAttrib2f tx2 = tx1 / scale2 + s2(0,1);

    ShAttrib4f L[4];

    // there are 4 edges with each one stored at each corner of 
    // the cell that x falls in.  We need to compute all of the 
    // them.  And then get the shortest one.
    // when computing distance, have to convert edge coords back
    // to real uv coords, because of distortion
    // these conversion can definitly be moved to preprocessing
    // part TODO
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

bool ShDoc::initFont(std::string filename, int small, int split)
{

  // small: num of small grids
  // split: num of small grids in one big grid
  font.loadFont(filename.c_str(), small, split);

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

  ftexture = ShArrayRect<ShAttrib4f>(width, height);
  flag.size(width, height);
  sprite1.size(smallgrid, smallgrid);
  sprite2.size(smallgrid, smallgrid);
 
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

  return true;
}

void ShDoc::string(int gnum, const char * str, float mg, float ng) {
  font.renderline(gnum, str, mg, ng);
}

void ShDoc::stringEnd() {
  font.stringEnd();
}

// =============================================================
// function: find the shortest distance of a point to edges
//           not only have to check the edges in the current cell,
//           but also in the previous cell horizontally, because 
//           the glyph may be wide such that it cross cell edge 
//           and fall into the next cell.
// x:        coordinates in the uv coord system
// =============================================================
ShAttrib4f ShDoc::shortestDis(ShAttrib2f x) {
  ShAttrib4f sprite_r[4];

  sprite_r[0] = sprite_dist(x, x);  // current cell
  sprite_r[1] = sprite_dist(x, x - ShAttrib2f(1.0/smallgrid,0.0)); //previous cell
  sprite_r[2] = sprite_dist(x, x - ShAttrib2f(2.0/smallgrid,0.0)); //previous cell

  // sometimes one glyph will cross two small grids,
  // so in each one small grid, have to check one
  // from the previous grid, and one from current 
  // grid
  ShAttrib4f r = cond(sprite_r[0](0) < sprite_r[1](0),sprite_r[0],sprite_r[1]);
  r = cond(r(0) < sprite_r[2](0),r,sprite_r[2]);
  r(2,3) = normalize(r(2,3));

  return r;
}

// =============================================================
// function: given the coordinates, find the isoantialiased color
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::isoAntialias(ShAttrib2f x,
		              ShColor3f m_color1, 
			      ShColor3f m_color2,
			      ShAttrib1f m_fw,
			      ShAttrib2f m_thres) 
{ 
  ShAttrib4f r = shortestDis(x);

  ShAttrib2f fw = fwidth(x);
  ShAttrib1f w = max(fw(0),fw(1))*m_fw;
  ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
  ShColor3f o = lerp(p,m_color2,m_color1);
  return o;
}

// =============================================================
// function: given the coordinates, find the anisoantiliased color
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::anisoAntialias(ShAttrib2f x,
		              ShColor3f m_color1, 
			      ShColor3f m_color2,
			      ShAttrib1f m_fw,
			      ShAttrib2f m_thres) 
{ 
  ShAttrib4f r = shortestDis(x);
  ShAttrib2f fw;
  fw(0) = dx(x) | r(2,3);
  fw(1) = dy(x) | r(2,3);
  ShAttrib1f w = length(fw)*m_fw;
  ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
  ShColor3f o = lerp(p,m_color2,m_color1);
 
  return o;
}

// =============================================================
// function: given the coordinates, find the aliased color
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::Alias(ShAttrib2f x,
		       ShColor3f m_color1, 
		       ShColor3f m_color2,
		       ShAttrib2f m_thres) 
{ 
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
 
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::isoAntiOutline(ShAttrib2f x,
		       ShColor3f m_color1, 
		       ShColor3f m_color2,
		       ShAttrib1f m_fw,
		       ShAttrib2f m_thres) 
{ 
  ShAttrib4f r = shortestDis(x);
  ShAttrib2f fw = fwidth(x);
  ShAttrib1f w = max(fw(0),fw(1))*m_fw;;
  ShAttrib2f p;
  p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
  p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
  ShColor3f o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
 
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::anisoAntiOutline(ShAttrib2f x,
		       ShColor3f m_color1, 
		       ShColor3f m_color2,
		       ShAttrib1f m_fw,
		       ShAttrib2f m_thres) 
{ 
  ShAttrib4f r = shortestDis(x);
  ShAttrib2f fw;;
  fw(0) = dx(x) | r(2,3);
  fw(1) = dy(x) | r(2,3);
  ShAttrib1f w = length(fw)*m_fw;
  ShAttrib2f p;
  p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
  p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
  ShColor3f o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
 
  return o;
}

// =============================================================
// function: given the coordinates, return the gradient
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::gradient(ShAttrib2f x,
		       ShColor3f m_vcolor1, 
		       ShColor3f m_vcolor2) 
{ 
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = 0.5 * (r(2) + 1.0) * m_vcolor1 
              + 0.5 * (r(3) + 1.0) * m_vcolor2;;
 
  return o;
}

// =============================================================
// function: given the coordinates, return the gradient
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::filterWidth(ShAttrib2f x)
{ 
  ShAttrib2f fw = fwidth(x);	      
  ShColor3f o = fw(0,1,0);
 
  return o;
}

// =============================================================
// function: given the coordinates, return the gradient
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::isoAntiPseudoOutline(ShAttrib2f x,
		ShColor3f m_color1,
		ShColor3f m_color2,
		ShAttrib1f m_fw,
		ShAttrib2f m_thres)
{
  ShAttrib4f r = shortestDis(x);
  ShAttrib2f fw = fwidth(x);;
  ShAttrib1f w = max(fw(0),fw(1))*m_fw;
  ShAttrib2f p;
  p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
  p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));;
  ShColor3f o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::anisoAntiPseudoOutline(ShAttrib2f x,
		ShColor3f m_color1,
		ShColor3f m_color2,
		ShAttrib1f m_fw,
		ShAttrib2f m_thres)
{
  ShAttrib4f r = shortestDis(x);
  ShAttrib2f fw;
  fw(0) = dx(x) | r(2,3);
  fw(1) = dy(x) | r(2,3);
  ShAttrib1f w = length(fw)*m_fw;
  ShAttrib2f p;
  p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
  p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));
  ShColor3f o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::biasSignedDis1(ShAttrib2f x,
		ShAttrib1f m_scale,
		ShColor3f m_vcolor1,
		ShColor3f m_vcolor2)
{
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = (0.5 + r(0) * m_scale)(0,0,0) 
              * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::biasSignedDis2(ShAttrib2f x,
		ShAttrib1f m_scale)
{
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = (0.5 + r(0) * m_scale)(0,0,0);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::signedDisMap(ShAttrib2f x,
		              ShAttrib1f m_scale,
			      ShColor3f m_vcolor1,
			      ShColor3f m_vcolor2)
{
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = (abs(r(0)) * m_scale)(0,0,0) 
          * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::biasSignPseudoMap(ShAttrib2f x,
		              ShAttrib1f m_scale,
			      ShColor3f m_vcolor1,
			      ShColor3f m_vcolor2)
{
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = (0.5 + r(1) * m_scale)(0,0,0) 
              * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
  return o;
}

// =============================================================
// function: given the coordinates, return the outline
// m_color1: color of the glyph interior
// m_color2: color of the glyph exterior
// =============================================================
ShColor3f ShDoc::biasSignPserdoMap(ShAttrib2f x,
		              ShAttrib1f m_scale)
{
  ShAttrib4f r = shortestDis(x);
  ShColor3f o = (0.5 + r(1) * m_scale)(0,0,0);
  return o;
}
>>>>>>> .r2836
