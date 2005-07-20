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
    m_psize(0),
    m_gridsize(0),
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

int ShFont::psize() const
{
  return m_psize;
}

int ShFont::gridsize() const
{
  return m_gridsize;
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

void ShFont::loadFont(const std::string& filename)
{
	try {
		m_memory = 0;

		int ifile;
		float coord;
		int flag;

		std::cerr << "openning the file " << filename.c_str() << std::endl; // xxx
		ifile = open(filename.c_str(), O_RDONLY);

		if(ifile > 0 ) {
		  	std::cerr << "file opened" << std::endl; // xxx

			read(ifile, &m_glyphcount, sizeof(int));
			read(ifile, &m_width, sizeof(int));
			read(ifile, &m_maxgwidth, sizeof(int));
			read(ifile, &m_maxgheight, sizeof(int));
			m_height = m_width;  // cause the texture is always a square

			std::cout << m_glyphcount << " ";
			std::cout << m_width << " " << m_height << " " ;
			std::cout << m_maxgwidth << " ";
			std::cout << m_maxgheight << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[5];
			m_memory[0] = new ShHostMemory(sizeof(int) * m_glyphcount * 11, SH_FLOAT);
			m_memory[1] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements, SH_FLOAT);
			m_memory[2] = new ShHostMemory(sizeof(int) * m_width * m_height, SH_FLOAT);

			int len = 11 * m_glyphcount;
			int temp;

			int gly=0, hadv=0, ymin=0, gw=0, gh=0, ox=0, oy=0, wino=0, hino=0;
			m_minhadvance = m_maxgwidth;

			for(int n=0; n<m_glyphcount; n++) {
				for(int k=0; k<11; k++) {
					read(ifile, &temp, sizeof(int));
					std::cerr << temp << " " ;
					coords(0)[n*11+k] = temp;

					// put glyph and its horizontal advance
					// into a map. will be used in function texture
					if(k==0) gly = temp;
					if(k==1) gw = temp;
					if(k==2) gh = temp;
					if(k==3) hadv = temp;
					if(k==5) ymin = temp;
					if(k==6) wino = temp;
					if(k==7) hino = temp;
					if(k==9) ox = temp;
					if(k==10) oy = temp;

					if(hadv < m_minhadvance) m_minhadvance = hadv;
				}
				hadvanceMap[gly] = hadv;
				gwidthMap[gly] = gw;
				gheightMap[gly] = gh;
				yminMap[gly] = ymin;
				// these maps are for kernning
				offsetx[gly] = ox;
				offsety[gly] = oy;
				winoctree[gly] = wino;
				hinoctree[gly] = hino;

			}
		
			std::cerr << std::endl;

			len = m_width * m_height * m_elements;

			for(int n=0; n<len; n++) {
				read(ifile, &coord, sizeof(float));
				//std::cerr << coord << " " ;
				coords(1)[n] = coord;
			}
			//std::cerr << std::endl;

			len = m_width * m_height;

			for(int n=0; n<len; n++) {
				read(ifile, &flag, sizeof(int));
				//std::cerr << flag << " " ;
				coords(2)[n] = flag;
			}
			//std::cerr << std::endl;
			
			while(1) {
				int left, right, kern;
				int rtn = read(ifile, &left, sizeof(int));
				if(rtn ==0) {
					//std::cout << "end of kern file" << std::endl;
					break;
				}
				read(ifile, &right, sizeof(int));
				read(ifile, &kern, sizeof(int));
				kmap[ Kernpair(left, right) ] = kern;
			}
			/*
			std::cout << " size ---------------" << kmap.size() << std::endl;
			for(std::map<Kernpair, int>::iterator i = kmap.begin(); i != kmap.end(); ++i) {
				std::cout << "get " << i->first.first() << " " << i->first.second();
				std::cout << " " << i->second << std::endl;
			}
			*/
		}
		close(ifile);
		std::cerr << "file closed" << std::endl; // xxx

	} catch(...){std::cerr << "glyph exception " << std::endl;}

	// input sprite 

	int num = 256;
	//int num = 128;
	m_psize = num;
	m_split = 16;
	m_gridsize = num/m_split;

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

		int gly = (int)sp[i*3];

		coords(3)[i*4] = sp[i*3+1];
		coords(3)[i*4+1] = sp[i*3+2];

		int j=0;
		for(j=0; j<m_glyphcount; j++) {
			if(coords(0)[j*11] == gly)
				break;
		}

		coords(3)[i*4+2] = coords(0)[j*11+1];
		coords(3)[i*4+3] = coords(0)[j*11+2];

		coords(4)[i*4] = coords(0)[j*11+9];
		coords(4)[i*4+1] = coords(0)[j*11+10];
		coords(4)[i*4+2] = coords(0)[j*11+6];
		coords(4)[i*4+3] = coords(0)[j*11+7];
	}

	// debug
	/*
	for(int i=0; i<len; i++) {
		std::cout << coords(3)[i*4] << " ";
		std::cout << coords(3)[i*4+1] << " ";
		std::cout << coords(3)[i*4+2] << " ";
		std::cout << coords(3)[i*4+3] << " ";
	}
	std::cout << std::endl;

	for(int i=0; i<len; i++) {
		std::cout << coords(4)[i*4] << " ";
		std::cout << coords(4)[i*4+1] << " ";
		std::cout << coords(4)[i*4+2] << " ";
		std::cout << coords(4)[i*4+3] << " ";
	}
	std::cout << std::endl;
	*/
}

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

	sx *= (float)m_maxgheight / (float)gw;
	sy *= (float)m_maxgheight / (float)gh;
	ex *= (float)m_maxgheight / (float)gw;
	ey *= (float)m_maxgheight / (float)gh;

	int xstart = ox + (int)(sx*ow);
	int ystart = oy + (int)(sy*oh);
	int xend = ox + (int)std::ceil(ex*ow);
	int yend = oy + (int)std::ceil(ey*oh);

	int flag = 0;
	for(int k = ystart; k<=yend; k++) {
		for(int l = xstart; l<=xend; l++) {

			int xlimit = ox + ow;
			int ylimit = oy + oh;

			if( k<ylimit && l<xlimit) {
				int index = k * m_width + l;
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

// gnum : number of glyphs in a line
// str:   string of glyphs
// mg:    starting x pos of glyph string in number of big grid
// ng:    starting y pos of glyph string in number of big grid
// one big grid is a container for one glyph, it may have mutliple
// small grids

void ShFont::renderline(int gnum, int * str, float mg, float ng, float * sp) {
	// x, y are the starting pos from 0-1
	float x = mg/m_gridsize;
	float y = ng/m_gridsize;
	float xx, yy;
	int mm, nn;

	for(int g=0; g<gnum; g++) {

		int curr = str[g];  // current glyph
		int next;           // next glyph
		int gw = gwidthMap[curr];
		int gh = gheightMap[curr];
		int ymin = yminMap[curr];
		int ox = offsetx[curr];
		int oy = offsety[curr];
		int ow = winoctree[curr];
		int oh = hinoctree[curr];

		float yshift = ymin - gh * MARGINRATIO;
	
		yy = y + 1.0/m_gridsize * yshift / (m_maxgheight * (1 + MARGINRATIO * 2));
		nn = (int)(yy * m_psize);

		xx = x - 1.0/m_gridsize * gw * MARGINRATIO / (m_maxgheight * (1 + MARGINRATIO * 2));
		mm = (int)(xx * m_psize);

		float sx = 0, sy = 0, ex, ey;

		std::cout << "gw " << gw << " m_maxgheight " << m_maxgheight << std::endl;

		// fill in one glyph
		for(int i=nn; i<=nn+m_split; i++) {

			ey = (((float)i + 1.0)/m_psize - yy) * m_gridsize;

			sx = 0;

			int mend = (int)((xx + (float)gw / (m_maxgheight * m_gridsize)) * m_psize);
			std::cout << "mend " << mend << std::endl;
			
			for(int j=mm; j<mend; j++) {

				ex = (((float)j + 1)/m_psize - xx) * m_gridsize;


				bool flag = getflag(sx, sy, ex, ey, gw, gh, ox, oy, ow, oh);

				if(flag) {
					sp[(i*m_psize+j)*3] = curr;
					sp[(i*m_psize+j)*3+1] = xx;
					sp[(i*m_psize+j)*3+2] = yy;
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
		float ratio = (float)adv / (m_maxgheight * (1 + MARGINRATIO * 2));

		// 0.02 is an offset such that the glyphs wont touch each other
		x += 1.0/m_gridsize * ratio + 0.001;
	}
}

void ShFont::texture(int num, float *sp) {

	int *array = new int[m_psize];
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

	array[0] = int('W');
	array[1] = int('e');
	array[2] = int(' ');
	array[3] = int('p');
	array[4] = int('r');
	array[5] = int('e');
	array[6] = int('s');
	array[7] = int('e');
	array[8] = int('n');
	array[9] = int('t');
	array[10] = int(' ');
	array[11] = int('a');
	array[12] = int(' ');
	array[13] = int('r');
	array[14] = int('e');
	array[15] = int('p');
	array[16] = int('r');
	array[17] = int('e');
	array[18] = int('s');
	array[19] = int('e');
	array[20] = int('n');
	array[21] = int('t');
	array[22] = int('a');
	array[23] = int('t');
	array[24] = int('i');
	array[25] = int('o');
	array[26] = int('n');
	array[27] = int(' ');
	array[28] = int('o');
	array[29] = int('f');

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
