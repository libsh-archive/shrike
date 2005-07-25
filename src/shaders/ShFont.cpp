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
#include "ShFont.hpp"
#include <string>
#include <cstring>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <png.h>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFERSIZE 256
#define PACK4
#define MARGINRATIO 0.1
#define LONG

ShFont::ShFont()
  : m_glyphcount(0), 
    m_width(0), 
    m_height(0), 
    m_elements(0), 
    m_smallgrid(0),
    m_biggrid(0),
    m_split(2),
    m_memory(0)
{
}

ShFont::~ShFont()
{
	// delete any memory allocated
}

int ShFont::glyphcount() const
{
  return m_glyphcount;
}

int ShFont::width() const
{
  return m_width;
}

int ShFont::height() const
{
  return m_height;
}

int ShFont::elements() const
{
  return m_elements;
}

int ShFont::smallgrid() const
{
  return m_smallgrid;
}

int ShFont::biggrid() const
{
  return m_biggrid;
}

int ShFont::maxgheight() const
{
  return m_maxgheight;
}

int ShFont::maxgwidth() const
{
  return m_maxgwidth;
}

int ShFont::minhadvance() const
{
  return m_minhadvance;
}

// =============================================================
// function: load the font from the file
//
// in this file, we read in info by steps
// 1: info about the whole font octree texture: width, height, etc
// 2: info about each glyph, including 11 items
// 3: info about edges 
// 4: info about flags
//
// filename:  the file where the font is stored
// totalgrid: num of small grids the texture has 
// split:     num of small grids in each big grid
// =============================================================
void ShFont::loadFont(const std::string& filename, int totalgrid, int split)
{
  try {
    m_smallgrid = totalgrid;
    m_split = split;
    m_memory = 0;

    int ifile;
    float coord;
    int flag;

    std::cerr << "openning the file " << filename.c_str() << std::endl; // xxx
    ifile = open(filename.c_str(), O_RDONLY);

    // if the file is opened successfully, read font info
    if(ifile > 0 ) {
      std::cerr << "file opened" << std::endl;

      // FIRST, read in info about the whole font
      // m_glyphcount: num of glyphs in the font 
      // m_width:      width of the octree texture 
      // m_height:     assume the octree texture is square  
      // m_maxgwidth:  max width of the glyph, not used 
      // m_maxheight:  max height of the glyph

      read(ifile, &m_glyphcount, sizeof(int));
      read(ifile, &m_width, sizeof(int));     
      m_height = m_width;                     
      read(ifile, &m_maxgwidth, sizeof(int));
      read(ifile, &m_maxgheight, sizeof(int));

      // debug info
      std::cout << m_glyphcount << " ";
      std::cout << m_width << " " << m_height << " " ;
      std::cout << m_maxgwidth << " ";
      std::cout << m_maxgheight << std::endl;

      // m_elements indicate max num of edges in each cell
      m_elements = 4;

      // buffer for edge coordinates and edge number
      m_memory = new ShHostMemoryPtr[5];
      // memory[0]: info about each glyph, such as width, height, etc.
      m_memory[0] = new ShHostMemory(sizeof(int) * m_glyphcount * 11, SH_FLOAT);
      // memory[1]: edge info
      m_memory[1] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements, SH_FLOAT);
      // memory[2]: flag if a grid is totally inside or outside of glyph
      m_memory[2] = new ShHostMemory(sizeof(int) * m_width * m_height, SH_FLOAT);
      // memory(3): glyph index, start x, y pos
      m_memory[3] = new ShHostMemory(sizeof(float) * m_smallgrid * m_smallgrid * 4, SH_FLOAT);
      // memory(4): glyph offset and size in octree texture
      m_memory[4] = new ShHostMemory(sizeof(int) * m_smallgrid * m_smallgrid * 4, SH_FLOAT);


      // SECOND, read in info about each glyph
      // each glyph has 11 info
      int len = 11 * m_glyphcount;
      int temp;

      int gly=0, hadv=0, ymin=0, gw=0, gh=0, ox=0, oy=0, wino=0, hino=0;
      m_minhadvance = m_maxgwidth;

      for(int n=0; n<m_glyphcount; n++) {
        for(int k=0; k<11; k++) {

          read(ifile, &temp, sizeof(int));
          std::cerr << temp << " " ;
	  // coords(0) returns pointer to memory(0)
          coords(0)[n*11+k] = temp;

          // put glyph and its horizontal advance
          // into a map. will be used in function texture
          if(k==0) gly = temp;    // the glyph index number
          if(k==1) gw = temp;     // the width of the glyph
          if(k==2) gh = temp;     // the height of the glyph
          if(k==3) hadv = temp;   // the horizontal advance
          if(k==5) ymin = temp;   // the min y value of the glyph
          if(k==6) wino = temp;   // the glyph width in the octree texture
          if(k==7) hino = temp;   // the glyph height in the octree texture
          if(k==9) ox = temp;     // the glyph x offset in the octree texture
          if(k==10) oy = temp;    // the glyph y offset in the octree texture

          if(hadv < m_minhadvance) m_minhadvance = hadv;
        }

	// put glyph info into maps, easy to use
        hadvanceMap[gly] = hadv;
        gwidthMap[gly] = gw;
        gheightMap[gly] = gh;
        yminMap[gly] = ymin;
        offsetx[gly] = ox;
        offsety[gly] = oy;
        winoctree[gly] = wino;
        hinoctree[gly] = hino;

      }
		
      std::cerr << std::endl;

      len = m_width * m_height * m_elements;

      // read the octree texture info into m_memory[1]
      // each pixel in the texture has 4 elements
      // for vertex coorinates
      for(int n=0; n<len; n++) {
        read(ifile, &coord, sizeof(float));
        coords(1)[n] = coord;
      }

      len = m_width * m_height;

      // read the flag (whether a cell is totally inside
      // or outside) into m_memory[2]
      for(int n=0; n<len; n++) {
        read(ifile, &flag, sizeof(int));
        coords(2)[n] = flag;
      }
			
      // read kerning info and put it into kmap
      while(1) {
        int left, right, kern;
        int rtn = read(ifile, &left, sizeof(int));
        if(rtn ==0) {
          break;
        }
        read(ifile, &right, sizeof(int));
        read(ifile, &kern, sizeof(int));
        kmap[ Kernpair(left, right) ] = kern;
      }
    }
    close(ifile);
    std::cerr << "file closed" << std::endl; // xxx

  } catch(...){std::cerr << "glyph exception " << std::endl;}

  // the following are for input char string

  m_biggrid = m_smallgrid/m_split;   // num of big grids
  sp = new float[m_smallgrid * m_smallgrid * 3];

}

void ShFont::stringEnd() {

  int len = m_smallgrid * m_smallgrid;

  for(int i=0; i<len*4; i++) {
    coords(3)[i] = 0;
    coords(4)[i] = 0;
  }

  for(int i=0; i<len; i++) {

    // get the glyph index
    int gly = (int)sp[i*3];

    // put the start x and y pos in m_memory(3)
    coords(3)[i*4] = sp[i*3+1];
    coords(3)[i*4+1] = sp[i*3+2];

    // find where the info about this glyph is in m_memory(0)
    int j=0;
    for(j=0; j<m_glyphcount; j++) {
      if(coords(0)[j*11] == gly)
      break;
    }

    // put width and height of the glyph in m_memory(3)
    coords(3)[i*4+2] = coords(0)[j*11+1];
    coords(3)[i*4+3] = coords(0)[j*11+2];

    // put glyph offset and width and height in the 
    // octree texture in m_memory(4)
    coords(4)[i*4] = coords(0)[j*11+9];
    coords(4)[i*4+1] = coords(0)[j*11+10];
    coords(4)[i*4+2] = coords(0)[j*11+6];
    coords(4)[i*4+3] = coords(0)[j*11+7];
  }
  delete[] sp;
}

// =============================================================
// function: this function should be changed in a while
// find out if a small cell occupied by a glyph is actually empty
// so that the next glyph can put its strokes there
// it is not looking in the whole glyph. Each glyph is separated
// into m_split*m_split small cells, it is looking for emptiness
// in each small cell.
// 
// sx: x starts pos of the small glyph cell
// sy: y starts pos of the small glyph cell
// ex: x end pos of the small glyph cell
// ey: y end pos of the small glyph cell
// gw: real width of the glyph
// gh: real height of the glyph
// ox: x offset of the glyph in the octree texture
// oy: y offset of the glyph in the octree texture
// ow: width of the glyph in the octree texture
// oh: height of the glyph in the octree texture
//
// return: 1: the cell is not empty; 0: the cell is empty
// =============================================================

bool ShFont:: getflag(float sx, 
		     float sy, 
		     float ex, 
		     float ey, 
		     int gw, 
		     int gh,
		     int ox, 
		     int oy, 
		     int ow, 
		     int oh) {

  // the glyph needs to be converted from the real texture domain 
  // into octree texture domain where the glyph info is stored.
	
  // m_maxheight/gw(gh) is the ratio of the glyph to the max height.
  // using maxheight as a standard, the glyph can be shorter or 
  // narrower or wider than it.  With the ratio, the glyph can be mapped 
  // from  the real glyph size to its corresponding storage square in the 
  // octree texture.
  // for example, suppose we have a glyph that is shorter than the max height,
  // then the glyph only uses a small part in a big grid(by default, we 
  // always assume each glyph takes a whole big grid). And this ratio is
  // how many percent is used by the smaller glyph.

  // sx, sy, ex, ey are the converting ratio for start and end positons
  // sx: the cell's x starting percentage in terms of the glyph's real width
  // sy: the cell's y starting percentage in terms of the glyph's real height
  // ex: the cell's x ending percentage in terms of the glyph's real width
  // ey: the cell's y ending percentage in terms of the glyph's real height
  sx *= (float)m_maxgheight / (float)gw;
  sy *= (float)m_maxgheight / (float)gh;
  ex *= (float)m_maxgheight / (float)gw;
  ey *= (float)m_maxgheight / (float)gh;

  // xstart: x starting pixel of the glyph in the octree texture.
  // ystart: y starting pxiel of the glyph in the octree texture.
  // xend:   x ending pixel of the glyph in the octree texture.
  // yend:   y ending pixel of the glyph in the octree texture.
  // to be conservative, we get floor for start pixel, and ceiling
  // for end pixel.

  int xstart = ox + (int)(sx*ow);  // this actually get the x floor int
  int ystart = oy + (int)(sy*oh);  // this actually get the y floor int
  int xend = ox + (int)std::ceil(ex*ow);  // this get the x ceiling int
  int yend = oy + (int)std::ceil(ey*oh);  // this get the y ceiling int

  // once we have the starting and end pixels in the octree texture,
  // we can search in the reactangle and find if the cell is empty.
  int flag = 0;
  for(int k = ystart; k<=yend; k++) {
    for(int l = xstart; l<=xend; l++) {

      int xlimit = ox + ow;
      int ylimit = oy + oh;

      // check only within the glyph's area
      // make sure not to search its neighbours.
      if( k<ylimit && l<xlimit) {
        // m_width: the width of the octree texture
        int index = k * m_width + l;
        // coords(2) stores the flags
        if(coords(2)[index] != 1) {
          flag = 1;
          break;
        }
      }
    }
  if(flag) break;
  }
  return flag;
}

// =============================================================
// function: render a string of characters in one row
//
// gnum : number of glyphs in a line
// str:   string of glyphs
// mg:    starting x pos of glyph string in number of big grid
// ng:    starting y pos of glyph string in number of big grid
//
// by default, one big grid is a container for one glyph, it may 
// have mutliple small grids. First, we assume all glyph takes
// the whole big grid, then we will adjust this according to 
// the glyph's real height and width.
// =============================================================

void ShFont::renderline(int gnum, const char * str, float mg, float ng) {

  // if coordinate of the whole area that is to be covered by 
  // the texture is from 0-1, x and y are the exact starting 
  // coordinates (from 0-1) of the string. It does take into 
  // accout of the margin. 
	
  float x = mg/m_biggrid;   // m_biggrid: num of big grids
  float y = ng/m_biggrid;
  float xx, yy;
  int mm, nn;

  // for each glyph
  for(int g=0; g<gnum; g++) {

    int curr = int(str[g]);             // current glyph
    int next;                      // next glyph
    int gw = gwidthMap[curr];      // width of the glyph
    int gh = gheightMap[curr];     // height of the glyph
    int ymin = yminMap[curr];      // the min y value of the glyph, can be negative
    int ox = offsetx[curr];        // the glyph x offset in the octree texture
    int oy = offsety[curr];        // the glyph y offset in the octree texture
    int ow = winoctree[curr];      // the glyph width in the octree texture
    int oh = hinoctree[curr];      // the glyph height in the octree texture

    // suppose there is a baseline, yshift is how much the glyph should be
    // below/above the base line, yy is the y coord where glyph starts.
    float yshift = ymin - gh * MARGINRATIO;    // remove the margin on the bottom
	
    // xx and yy are the actually starting pos of the glyph after taking
    // into account of the margin. 
    // yy similar to in xx, refer to following explanation for xx and mm
    // margin move the glyph up, and min y(if negative) move the glyph down
    // y axis goes from below to above
    // so for margin, we need to deduct it; for ymin, we need to add it
    // because if it is below baseline, ymin is negative already, and vice versa
    // suppose the highest glyph+2margin occupy the whole big grid

    // yy: y coordinate of 0-1 where glyph starts from exactly
    // nn: the num of small grid that the glyph starts from in y axis
    yy = y + 1.0/m_biggrid * yshift / (m_maxgheight * (1 + MARGINRATIO * 2));
    nn = (int)(yy * m_smallgrid);

    // 1.0/m_biggrid is how many percent each big grid occupies 
    // this removes the MARGINRATION part from the glyph
    // the wider the glyph, the bigger the margin
    // divided by the maxheight+2Margin is also a percentage thing
    // maybe divided by other constant is ok too, as long as it is constant
    // m_smallgrid is the total number of small grids
		
    // xx: x coordinate of 0-1 where glyph starts from exactly
    // mm: the num of small grid that the glyph starts from in x axis
    xx = x - 1.0/m_biggrid * gw * MARGINRATIO / (m_maxgheight * (1 + MARGINRATIO * 2));
    mm = (int)(xx * m_smallgrid);

    // sx, sy: the starting percentage of the small grid in terms of the big grid
    // ex, ey: the ending percentage of the small grid in terms of the big grid
    float sx = 0, sy = 0, ex, ey;

    std::cout << "gw " << gw << " m_maxgheight " << m_maxgheight << std::endl;

    // m_split: num of small grids in one big grid
    // m_biggrid: num of big grids
    // fill in the rectangle the glyph is going to have
    // in y direction, there are m_split small grids
    // in x direction, there are mend small grids
    // vertically, no glyph can exceed the m_split
    // horizontaly, some glyphs are wider than the biggest height
    for(int i=nn; i<=nn+m_split; i++) {

      // end y percentage of each small cell in the big grid
      // since the end percentage is the beginning of the next
      // small cell, that is why we need "+1.0"
      // after -yy, we got the percentage of the end pos in the
      // whole area. "*m_biggrid makes it percentage in one big
      // grid. 
      ey = (((float)i + 1.0)/m_smallgrid - yy) * m_biggrid;

      // starting percentage of each small cell in the glyph
      // sy is initialized out of the loop, cause it is constant 
      // in each row.
      // since s
      sx = 0;

      // since the glyph may be wider than the biggest height
      // we need to recalculate the real num of small grids a glyph
      // need horizontally. mend is where the small grids end.
      int mend = (int)((xx + (float)gw / (m_maxgheight * m_biggrid)) * m_smallgrid);
      std::cout << "mend " << mend << std::endl;
			
      for(int j=mm; j<mend; j++) {

	// percentage where a small cell ends in the big grid
	ex = (((float)j + 1)/m_smallgrid - xx) * m_biggrid;


	// check if this small grid is empty
	// this function should be changed
	// and use the distance instead
	// bool flag = getflag(sx, sy, ex, ey, gw, gh, ox, oy, ow, oh);

	// if the small grid is not empty, put in info of the current 
	// glyph. Otherwise, leave it blank.
	// in sp, we have glyph index, exact x and y start pos of the glyph
	bool flag = 1;
	if(flag) {
          sp[(i*m_smallgrid+j)*3] = curr;
	  sp[(i*m_smallgrid+j)*3+1] = xx;
	  sp[(i*m_smallgrid+j)*3+2] = yy;
	}

      sx = ex;
      }

      sy = ey;
    }

    // get the kerning info between current glyph and the next
    if(g<gnum-1)
      next = int(str[g+1]);
    else next = 0;

    int kernvalue = kmap[ Kernpair(curr, next) ];

    // get the advance in x without kerning
    int adv;
    // this 4 can be changed TODO
    if(curr == int(' ')) adv = m_maxgheight/3.345;
    else adv = hadvanceMap[curr] + kernvalue/4.0;  // advance + kerning info

    // move x forward, keep y unchanged for now
    // we are using the maxhieght+2magines as the consistent 
    // denominator.
    float ratio = (float)adv / (m_maxgheight * (1 + MARGINRATIO * 2));

    x += 1.0/m_biggrid * ratio;
  }
}

const float* ShFont::coords(int i) const
{
  if (!m_memory) return 0;
  return reinterpret_cast<const float*>(m_memory[i]->hostStorage()->data());
}

float* ShFont::coords(int i)
{
  if (!m_memory) return 0;
  return reinterpret_cast<float*>(m_memory[i]->hostStorage()->data());
}

ShMemoryPtr ShFont::memory(int i)
{
  return m_memory[i];
}

ShPointer<const ShMemory> ShFont::memory(int i) const
{
  return m_memory[i];
}
