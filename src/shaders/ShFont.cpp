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
    m_memory(0),
    m_smallgrid(0),
    m_biggrid(0),
    m_split(2)
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

  // input sprite 

  int num = m_smallgrid;     // total num of small grids
  m_biggrid = num/m_split;   // num of big grids

  int len = num * num;
  float * sp = new float[len * 3];

  texture(num, sp);

  // for sprite
  m_memory[3] = new ShHostMemory(sizeof(float) * len * 4, SH_FLOAT);
  m_memory[4] = new ShHostMemory(sizeof(int) * len * 4, SH_FLOAT);

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

void ShFont::renderline(int gnum, int * str, float mg, float ng, float * sp) {

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

    int curr = str[g];             // current glyph
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
	bool flag = getflag(sx, sy, ex, ey, gw, gh, ox, oy, ow, oh);

	// if the small grid is not empty, put in info of the current 
	// glyph. Otherwise, leave it blank.
	// in sp, we have glyph index, exact x and y start pos of the glyph
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
      next = str[g+1];
    else next = 0;

    int kernvalue = kmap[ Kernpair(curr, next) ];

    // get the advance in x without kerning
    int adv;
    if(curr == int(' ')) adv = m_maxgheight/4;
    else adv = hadvanceMap[curr] + kernvalue;  // advance + kerning info

    // move x forward, keep y unchanged for now
    // since we are using the maxhieght+2magines as the consistent 
    // denominator, we might as well use it here.
    float ratio = (float)adv / (m_maxgheight * (1 + MARGINRATIO * 2));

    // 0.02 is an offset such that the glyphs wont touch each other
    x += 1.0/m_biggrid * ratio + 0.001;
  }
}

void ShFont::texture(int num, float *sp) {

	int *array = new int[m_smallgrid];
	/*

	array[0] = int('P');
	array[1] = int('r');
	array[2] = int('e');
	array[3] = int('m');
	array[4] = int('a');
	array[5] = int('t');
	array[6] = int('u');
	array[7] = int('r');
	array[8] = int('e');

	renderline(9, array, 0.25, 6, sp);

	array[0] = int('o');
	array[1] = int('p');
	array[2] = int('t');
	array[3] = int('i');
	array[4] = int('m');
	array[5] = int('i');
	array[6] = int('z');
	array[7] = int('a');
	array[8] = int('t');
	array[9] = int('i');
	array[10] = int('o');
	array[11] = int('n');

	renderline(12, array, 0.25, 4.5, sp);

	array[0] = int('i');
	array[1] = int('s');
	array[2] = int(' ');
	array[3] = int('t');
	array[4] = int('h');
	array[5] = int('e');
	array[6] = int(' ');
	array[7] = int('r');
	array[8] = int('o');
	array[9] = int('o');
	array[10] = int('t');

	renderline(11, array, 0.25, 3, sp);

	array[0] = int('o');
	array[1] = int('f');
	array[2] = int(' ');
	array[3] = int('a');
	array[4] = int('l');
	array[5] = int('l');
	array[6] = int(' ');
	array[7] = int('e');
	array[8] = int('v');
	array[9] = int('i');
	array[10] = int('l');

	renderline(11, array, 0.25, 1.5, sp);
	*/

	std::string str = "We Present a representation of";

	const char * s = str.c_str();
	int len = str.length();
	for (int i=0; i<len; i++) {
	  array[i] = (int)s[i];
	}

	renderline(30, array, 0.25, 15, sp);

	array[0] = int('f');
	array[1] = int('o');
	array[2] = int('n');
	array[3] = int('t');
	array[4] = int(' ');
	array[5] = int('g');
	array[6] = int('l');
	array[7] = int('y');
	array[8] = int('p');
	array[9] = int('h');
	array[10] = int('s');
	array[11] = int(' ');
	array[12] = int('s');
	array[13] = int('u');
	array[14] = int('i');
	array[15] = int('t');
	array[16] = int('a');
	array[17] = int('b');
	array[18] = int('l');
	array[19] = int('e');
	array[20] = int(' ');
	array[21] = int('f');
	array[22] = int('o');
	array[23] = int('r');
	array[24] = int(' ');
	array[25] = int('r');
	array[26] = int('e');
	array[27] = int('a');
	array[28] = int('l');
	array[29] = int('t');
	array[30] = int('i');
	array[31] = int('m');
	array[32] = int('e');

	renderline(33, array, 0.25, 13.5, sp);

	array[0] = int('s');
	array[1] = int('c');
	array[2] = int('a');
	array[3] = int('l');
	array[4] = int('a');
	array[5] = int('b');
	array[6] = int('l');
	array[7] = int('e');
	array[8] = int(' ');
	array[9] = int('t');
	array[10] = int('e');
	array[11] = int('x');
	array[12] = int('t');
	array[13] = int(' ');
	array[14] = int('r');
	array[15] = int('e');
	array[16] = int('n');
	array[17] = int('d');
	array[18] = int('e');
	array[19] = int('r');
	array[20] = int('i');
	array[21] = int('n');
	array[22] = int('g');
	array[23] = int(' ');
	array[24] = int('o');
	array[25] = int('n');
	array[26] = int(' ');
	array[27] = int('G');
	array[28] = int('P');
	array[29] = int('U');
	array[30] = int('s');
	array[31] = int('.');

	renderline(32, array, 0.25, 12, sp);

	array[0] = int('C');
	array[1] = int('o');
	array[2] = int('n');
	array[3] = int('t');
	array[4] = int('o');
	array[5] = int('u');
	array[6] = int('r');
	array[7] = int('s');
	array[8] = int(' ');
	array[9] = int('a');
	array[10] = int('n');
	array[11] = int('d');
	array[12] = int(' ');
	array[13] = int('s');
	array[14] = int('h');
	array[15] = int('a');
	array[16] = int('r');
	array[17] = int('p');
	array[18] = int(' ');
	array[19] = int('f');
	array[20] = int('e');
	array[21] = int('a');
	array[22] = int('t');
	array[23] = int('u');
	array[24] = int('r');
	array[25] = int('e');
	array[26] = int('s');
	array[27] = int(' ');
	array[28] = int('c');
	array[29] = int('a');
	array[30] = int('n');

	renderline(31, array, 0.25, 10.5, sp);

	array[0] = int('b');
	array[1] = int('e');
	array[2] = int(' ');
	array[3] = int('e');
	array[4] = int('x');
	array[5] = int('a');
	array[6] = int('c');
	array[7] = int('t');
	array[8] = int('l');
	array[9] = int('y');
	array[10] = int(' ');
	array[11] = int('r');
	array[12] = int('e');
	array[13] = int('c');
	array[14] = int('o');
	array[15] = int('n');
	array[16] = int('s');
	array[17] = int('t');
	array[18] = int('r');
	array[19] = int('u');
	array[20] = int('c');
	array[21] = int('t');
	array[22] = int('e');
	array[23] = int('d');
	array[24] = int(' ');
	array[25] = int('u');
	array[26] = int('s');
	array[27] = int('i');
	array[28] = int('n');
	array[29] = int('g');
	array[30] = int(' ');
	array[31] = int('a');

	renderline(32, array, 0.25, 9, sp);

	array[0] = int('c');
	array[1] = int('o');
	array[2] = int('n');
	array[3] = int('s');
	array[4] = int('t');
	array[5] = int('a');
	array[6] = int('n');
	array[7] = int('t');
	array[8] = int(' ');
	array[9] = int('a');
	array[10] = int('m');
	array[11] = int('o');
	array[12] = int('u');
	array[13] = int('n');
	array[14] = int('t');
	array[15] = int(' ');
	array[16] = int('o');
	array[17] = int('f');
	array[18] = int(' ');
	array[19] = int('c');
	array[20] = int('o');
	array[21] = int('m');
	array[22] = int('p');
	array[23] = int('u');
	array[24] = int('t');
	array[25] = int('a');
	array[26] = int('t');
	array[27] = int('i');
	array[28] = int('o');
	array[29] = int('n');

	renderline(30, array, 0.25, 7.5, sp);

	array[0] = int('t');
	array[1] = int('i');
	array[2] = int('m');
	array[3] = int('e');
	array[4] = int(' ');
	array[5] = int('p');
	array[6] = int('e');
	array[7] = int('r');
	array[8] = int(' ');
	array[9] = int('p');
	array[10] = int('i');
	array[11] = int('x');
	array[12] = int('e');
	array[13] = int('l');
	array[14] = int('.');
	array[15] = int(' ');
	array[16] = int('A');
	array[17] = int(' ');
	array[18] = int('c');
	array[19] = int('o');
	array[20] = int('m');
	array[21] = int('b');
	array[22] = int('i');
	array[23] = int('n');
	array[24] = int('a');
	array[25] = int('t');
	array[26] = int('i');
	array[27] = int('o');
	array[28] = int('n');
	array[29] = int(' ');
	array[30] = int('o');
	array[31] = int('f');

	renderline(32, array, 0.25, 6, sp);

	array[0] = int('t');
	array[1] = int('e');
	array[2] = int('x');
	array[3] = int('t');
	array[4] = int('u');
	array[5] = int('r');
	array[6] = int('e');
	array[7] = int(' ');
	array[8] = int('d');
	array[9] = int('a');
	array[10] = int('t');
	array[11] = int('a');
	array[12] = int(' ');
	array[13] = int('a');
	array[14] = int('n');
	array[15] = int('d');
	array[16] = int(' ');
	array[17] = int('p');
	array[18] = int('r');
	array[19] = int('o');
	array[20] = int('c');
	array[21] = int('e');
	array[22] = int('d');
	array[23] = int('u');
	array[24] = int('r');
	array[25] = int('a');
	array[26] = int('l');
	array[27] = int(' ');
	array[28] = int('c');
	array[29] = int('o');
	array[30] = int('m');
	//array[31] = int('-');

	renderline(31, array, 0.25, 4.5, sp);

	array[0] = int('p');
	array[1] = int('u');
	array[2] = int('t');
	array[3] = int('a');
	array[4] = int('t');
	array[5] = int('i');
	array[6] = int('o');
	array[7] = int('n');
	array[8] = int(' ');
	array[9] = int('i');
	array[10] = int('s');
	array[11] = int(' ');
	array[12] = int('u');
	array[13] = int('s');
	array[14] = int('e');
	array[15] = int('d');
	array[16] = int(' ');
	array[17] = int('t');
	array[18] = int('o');
	array[19] = int(' ');
	array[20] = int('r');
	array[21] = int('e');
	array[22] = int('c');
	array[23] = int('r');
	array[24] = int('e');
	array[25] = int('a');
	array[26] = int('t');
	array[27] = int('e');
	array[28] = int(' ');
	array[29] = int('t');
	array[30] = int('h');
	array[31] = int('e');

	renderline(32, array, 0.25, 3, sp);

	array[0] = int('s');
	array[1] = int('i');
	array[2] = int('g');
	array[3] = int('n');
	array[4] = int('e');
	array[5] = int('d');
	array[6] = int(' ');
	array[7] = int('d');
	array[8] = int('i');
	array[9] = int('s');
	array[10] = int('t');
	array[11] = int('a');
	array[12] = int('n');
	array[13] = int('c');
	array[14] = int('e');
	array[15] = int(' ');
	array[16] = int('f');
	array[17] = int('i');
	array[18] = int('e');
	array[19] = int('l');
	array[20] = int('d');
	array[21] = int(' ');
	array[22] = int('a');
	array[23] = int('n');
	array[24] = int('d');
	array[25] = int(' ');
	array[26] = int('i');
	array[27] = int('t');
	array[28] = int('s');
	array[29] = int(' ');
	array[30] = int('g');
	array[31] = int('r');
	array[32] = int('a');
	//array[34] = int('-');

	renderline(33, array, 0.25, 1.5, sp);

	array[0] = int('d');
	array[1] = int('i');
	array[2] = int('e');
	array[3] = int('n');
	array[4] = int('t');
	array[5] = int('.');

	renderline(6, array, 0.25, 0, sp);

	delete[] array;
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
