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
#include "ShGlyph.hpp"
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

ShGlyph::ShGlyph()
  : m_width(0), 
    m_height(0), 
    m_elements(0), 
    m_edges(0), 
    m_memory(0), 
    m_edgenum(0)
{
}

ShGlyph::~ShGlyph()
{
	// delete any memory allocated
}

int ShGlyph::width() const
{
  return m_width;
}

int ShGlyph::height() const
{
  return m_height;
}

int ShGlyph::edges() const
{
  return m_edges;
}

int ShGlyph::elements() const
{
  return m_elements;
}

float ShGlyph::halfx() const
{
  return m_halfx;
}

float ShGlyph::halfy() const
{
  return m_halfy;
}

void ShGlyph::loadGlyph(const std::string& filename)
{
#ifdef NOPACK
	try {

		m_memory = 0;
		m_edgenum = 0;

		int ifile;
		int tindex, tnum;
		float coord;

		ifile = open(filename.c_str(), O_RDONLY);

		if(ifile > 0 ) {
			read(ifile, &m_width, sizeof(int));
			read(ifile, &m_height, sizeof(int));
			read(ifile, &m_edges, sizeof(int));
			read(ifile, &m_halfx, sizeof(float));
			read(ifile, &m_halfy, sizeof(float));
			std::cout << m_width << " " << m_height << " " << m_edges << " ";
			std::cout << m_halfx << " " << m_halfy << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[m_edges];
			for(int i=0; i<m_edges; i++) {
				m_memory[i] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);
			}
			// integer array should be enough for number of edges in each cell
			// but Sh may not work for integer texture
			m_edgenum = new ShHostMemory(sizeof(float) * m_width * m_height);

			int len = m_width * m_height;

			for(int n=0; n<len; n++) {
				read(ifile, &tindex, sizeof(int));
				read(ifile, &tnum, sizeof(int));
				std::cout << "index : " << tindex << " num : " << tnum << std::endl;
				edgenum()[tindex] = (float)tnum;
			
				for(int l=0; l<m_edges; l++) {
					for(int k=0; k<m_elements; k++){
						read(ifile, &coord, sizeof(float));
						//std::cout << coord << " " ;
						int index = n * m_elements + k;
						coords(l)[index] = coord;
					}
					//std::cout << std::endl;
				}
		
			}
		}
		close(ifile);
	} catch(...){std::cerr << "glyph exception " << std::endl;}
#endif
#ifdef PACK9
	try {

		m_memory = 0;
		m_edgenum = 0;

		int ifile;
		float coord;

		ifile = open(filename.c_str(), O_RDONLY);

		if(ifile > 0 ) {
			read(ifile, &m_width, sizeof(int));
			read(ifile, &m_height, sizeof(int));
			std::cout << m_width << " " << m_height << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[1];
			m_memory[0] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);

			int len = m_width * m_height * m_elements;

			for(int n=0; n<len; n++) {
				read(ifile, &coord, sizeof(float));
				coords(0)[n] = coord;
			}
		}
		close(ifile);
	} catch(...){std::cerr << "glyph exception " << std::endl;}
#endif
	
#ifdef PACK4
	std::cerr << "For pack 4" << std::endl; // xxx
	try {
		m_memory = 0;
		m_edgenum = 0;

		int ifile;
		float coord;
		int flag;

		std::cerr << "Trying to open the file" << std::endl; // xxx
		ifile = open(filename.c_str(), O_RDONLY);

		if(ifile > 0 ) {
		  std::cerr << "opened ok" << std::endl; // xxx
			read(ifile, &m_width, sizeof(int));
			read(ifile, &m_height, sizeof(int));
			read(ifile, &m_edges, sizeof(int));
			std::cout << m_width << " " << m_height << " " << m_edges << std::endl;

			m_elements = 4;

			// buffer for edge coordinates and edge number
			m_memory = new ShHostMemoryPtr[2];
			m_memory[0] = new ShHostMemory(sizeof(float) * m_width * m_height * m_elements);
			m_memory[1] = new ShHostMemory(sizeof(int) * m_width * m_height);

			int len = m_width * m_height * m_elements;

			std::cerr << "read " << m_width << " x " << m_height << std::endl; // xxx
			std::cerr << "Starting cycle" << std::endl; // xxx
			for(int n=0; n<len; n++) {
				read(ifile, &coord, sizeof(float));
				coords(0)[n] = coord;
			}

			len = m_width * m_height;

			for(int n=0; n<len; n++) {
				read(ifile, &flag, sizeof(int));
				coords(1)[n] = flag;
			}
		}
		close(ifile);
	} catch(...){std::cerr << "glyph exception " << std::endl;}
#endif
}

const float* ShGlyph::coords(int i) const
{
  if (!m_memory) return 0;
  return reinterpret_cast<const float*>(m_memory[i]->hostStorage()->data());
}

const float* ShGlyph::edgenum() const
{
  if (!m_edgenum) return 0;
  return reinterpret_cast<const float*>(m_edgenum->hostStorage()->data());
}

float* ShGlyph::coords(int i)
{
  if (!m_memory) return 0;
  return reinterpret_cast<float*>(m_memory[i]->hostStorage()->data());
}

float* ShGlyph::edgenum()
{
  if (!m_edgenum) return 0;
  return reinterpret_cast<float*>(m_edgenum->hostStorage()->data());
}

ShMemoryPtr ShGlyph::memory(int i)
{
  return m_memory[i];
}

ShPointer<const ShMemory> ShGlyph::memory(int i) const
{
  return m_memory[i];
}

ShMemoryPtr ShGlyph::edge()
{
  return m_edgenum;
}

ShPointer<const ShMemory> ShGlyph::edge() const
{
  return m_edgenum;
}
