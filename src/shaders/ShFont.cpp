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
			m_height = m_width;  // cause the texture is always a square

			std::cout << m_glyphcount << " ";
			std::cout << m_width << " " << m_height << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[3];
			m_memory[0] = new ShHostMemory(sizeof(int) * m_glyphcount * 6);
			m_memory[1] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);
			m_memory[2] = new ShHostMemory(sizeof(int) * m_width * m_height);

			int len = 6 * m_glyphcount;
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
			
			/*
			// debug
			for(int i=0; i<m_height; i++) {
				for(int j=0; j<m_width; j++) {
					std::cerr << coords(2)[i*m_width+j] << " " ;
				}
				std::cerr << std::endl;
			}
			*/
		}
		close(ifile);
		std::cerr << "file closed" << std::endl; // xxx
	} catch(...){std::cerr << "glyph exception " << std::endl;}
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
