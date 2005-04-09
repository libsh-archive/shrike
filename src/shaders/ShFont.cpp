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
/*
#include "ShException.hpp"
#include "ShError.hpp"
#include "ShDebug.hpp"
*/
#define BUFFERSIZE 256
#define PACK4

ShFont::ShFont()
  : m_glyphcount(0), 
    m_width(0), 
    m_height(0), 
    m_elements(0), 
    m_memory(0),
    m_psize(0)
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

int ShFont::maxgheight() const
{
  return m_maxgheight;
}

int ShFont::maxgwidth() const
{
  return m_maxgwidth;
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
			std::cout << m_maxgwidth << std::endl;
			std::cout << m_maxgheight << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[5];
			m_memory[0] = new ShHostMemory(sizeof(int) * m_glyphcount * 8);
			m_memory[1] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);
			m_memory[2] = new ShHostMemory(sizeof(int) * m_width * m_height);

			int len = 8 * m_glyphcount;
			int temp;

			for(int n=0; n<len; n++) {
				read(ifile, &temp, sizeof(int));
				std::cerr << temp << " " ;
				coords(0)[n] = temp;
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

	int num = 4;
	/*
	std::cout << "Please input grid number: " << std::endl;
	std::cin >> num;
	*/
	m_psize = num;

	int len = num * num;
	float * sp = new float[len * 3];

	/*
	std::cout << "Please input glyph number, offsetx and offsety" << std::endl;

	for(int i=0; i<len; i++) {
		std::cin >> sp[i*3] >> sp[i*3+1] >> sp[i*3+2];
	}
	*/
	sp[0] = int('i');
	sp[1] = 0;
	sp[2] = 0;
	sp[3] = 0;
	sp[4] = 0.25;
	sp[5] = 0;
	sp[6] = 0;
	sp[7] = 0.5;
	sp[8] = 0;
	sp[9] = int('D');
	sp[10] = 0.75;
	sp[11] = 0;
	sp[12] = int('D');
	sp[13] = 0;
	sp[14] = 0.25;
	sp[15] = int('D');
	sp[16] = 0.35;
	sp[17] = 0.25;
	sp[18] = int('k');
	sp[19] = 0.5;
	sp[20] = 0.25;
	sp[21] = int('e');
	sp[22] = 0.75;
	sp[23] = 0.25;
	sp[24] = int('a');
	sp[25] = 0;
	sp[26] = 0.5;
	sp[27] = int('m');
	sp[28] = 0.25;
	sp[29] = 0.5;
	sp[30] = int('d');
	sp[31] = 0.5;
	sp[32] = 0.5;
	sp[33] = int('d');
	sp[34] = 0.75;
	sp[35] = 0.5;
	sp[36] = int('f');
	sp[37] = 0;
	sp[38] = 0.75;
	sp[39] = int('h');
	sp[40] = 0.25;
	sp[41] = 0.75;
	sp[42] = int('i');
	sp[43] = 0.5;
	sp[44] = 0.75;
	sp[45] = int('j');
	sp[46] = 0.75;
	sp[47] = 0.75;

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
			if(coords(0)[j*8] == gly)
				break;
		}

		coords(3)[i*4+2] = coords(0)[j*8+1];
		coords(3)[i*4+3] = coords(0)[j*8+2];

		coords(4)[i*4] = coords(0)[j*8+6];
		coords(4)[i*4+1] = coords(0)[j*8+7];
		coords(4)[i*4+2] = coords(0)[j*8+3];
		coords(4)[i*4+3] = coords(0)[j*8+4];
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
