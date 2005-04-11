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
#include <map>
/*
#include "ShException.hpp"
#include "ShError.hpp"
#include "ShDebug.hpp"
*/
#define BUFFERSIZE 256
#define PACK4
#define MARGINRATIO 0.1

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
			m_memory[0] = new ShHostMemory(sizeof(int) * m_glyphcount * 11);
			m_memory[1] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);
			m_memory[2] = new ShHostMemory(sizeof(int) * m_width * m_height);

			int len = 11 * m_glyphcount;
			int temp;

			int gly=0, hadv=0, ymin=0, gw=0, gh=0;
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

					if(hadv < m_minhadvance) m_minhadvance = hadv;
				}
				hadvanceMap[gly] = hadv;
				gwidthMap[gly] = gw;
				gheightMap[gly] = gh;
				yminMap[gly] = ymin;
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
			
		}
		close(ifile);
		std::cerr << "file closed" << std::endl; // xxx

	} catch(...){std::cerr << "glyph exception " << std::endl;}

	// input sprite 

	int num = 64;
	m_psize = num;
	m_split = 8;
	m_gridsize = num/m_split;

	int len = num * num;
	float * sp = new float[len * 3];

	texture(num, sp);

	// for sprite
	m_memory[3] = new ShHostMemory(sizeof(float) * len * 4);
	m_memory[4] = new ShHostMemory(sizeof(int) * len * 4);

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

		int gw = gwidthMap[str[g]];
		int gh = gheightMap[str[g]];
		int ymin = yminMap[str[g]];

		int yshift = ymin - gh * MARGINRATIO;
	
		yy = y + 1.0/m_gridsize * yshift / (m_maxgheight * (1 + MARGINRATIO * 2));
		nn = (int)(yy * m_psize);

		xx = x - 1.0/m_gridsize * gw * MARGINRATIO / (m_maxgheight * (1 + MARGINRATIO * 2));
		mm = (int)(xx * m_psize);

		// fill in one glyph
		for(int i=nn; i<nn+m_split; i++) {
			for(int j=mm; j<mm+m_split; j++) {
				sp[(i*m_psize+j)*3] = str[g];
				sp[(i*m_psize+j)*3+1] = xx;
				sp[(i*m_psize+j)*3+2] = yy;
			}
		}

		int adv;
		if(str[g] == int(' ')) adv = m_maxgheight/4;
		else adv = hadvanceMap[str[g]];

		// move x forward, keep y unchanged for now
		float ratio = (float)adv / (m_maxgheight * (1 + MARGINRATIO * 2));

		x += 1.0/m_gridsize * ratio;

	}
}

void ShFont::texture(int num, float *sp) {

	int *array = new int[m_psize];

	array[0] = int('P');
	array[1] = int('r');
	array[2] = int('e');
	array[3] = int('m');
	array[4] = int('a');
	array[5] = int('t');
	array[6] = int('u');
	array[7] = int('r');
	array[8] = int('e');

	renderline(9, array, 0.5, 6, sp);

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

	renderline(12, array, 0.5, 4.5, sp);

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

	renderline(11, array, 0.5, 3, sp);

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

	renderline(11, array, 0.5, 1.5, sp);

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
