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
#include <iostream>
#include <string>
#include <sh/sh.hpp>
#include "HDRImage.hpp"

#define USE_EXR 0

#if USE_EXR
#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfRgba.h>
#include <ImfArray.h>
#include <ImathBox.h>


using namespace Imath;
using namespace Imf;
#endif

using namespace SH;

HDRImage::HDRImage()
				: m_width(0), m_height(0), m_elements(0) {
	m_image = ShImage(0,0,0);
}


HDRImage::HDRImage(int width, int height, int depth) {
	m_width = width;
	m_height = height;
	m_elements = depth;
	m_image = ShImage(width,height,depth);
}

HDRImage::HDRImage(const HDRImage& other) {
	m_width = other.m_width;
	m_height = other.m_height;
	m_elements = other.m_elements;
	m_image = ShImage(other.m_image);
}

HDRImage::~HDRImage() {
}

HDRImage& HDRImage::operator=(const HDRImage& other)
{
  m_width = other.m_width;
  m_height = other.m_height;
  m_elements = other.m_elements;
  m_image = other.m_image;
  return *this;
}

#if USE_EXR
// load an OpenEXR file with the library from ILM
void HDRImage::loadEXR(const char filename[]) {
	RgbaInputFile file (filename);
	Box2i dw = file.dataWindow();
	m_width  = dw.max.x - dw.min.x + 1;
	m_height = dw.max.y - dw.min.y + 1;
	m_elements = 4;

	ShImage img(m_width, m_height, 4); // to save the image
					
  Array2D<Rgba> pixels;
	pixels.resizeErase (m_height,m_width);
	file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * m_width, 1, m_width);
	file.readPixels (dw.min.y, dw.max.y); // read the exr image

	for(int j=0 ; j < m_height ; j++) { // save the buffer in the ShImage
		for(int i=0 ; i < m_width ; i++) {
			const Rgba inPixel = pixels[j][i];
			img(i,j,0) = inPixel.r;
			img(i,j,1) = inPixel.g;
			img(i,j,2) = inPixel.b;
			img(i,j,3) = inPixel.a;
		}
	}
	m_image = img;
}

// save m_image to and OpenEXR file
void HDRImage::saveEXR(const char filename[]) {
	RgbaOutputFile file (filename, m_width, m_height, WRITE_RGBA);
	std::cout<<m_width<<" "<<m_height<<std::endl;
	Array2D<Rgba> pixels;
	pixels.resizeErase (m_height,m_width);
	for(int j=0 ; j < m_height ; j++) { // fill the buffer with the image values
		for(int i=0 ; i < m_width ; i++) {
			Rgba inPixel;
			inPixel.r = m_image(i,j,0);
			inPixel.g = m_image(i,j,1);
			inPixel.b = m_image(i,j,2);
			inPixel.a = m_image(i,j,3);
			pixels[j][i] = inPixel;
		}
	}
	file.setFrameBuffer (&pixels[0][0], 1, m_width);
	file.writePixels (m_height);
}
#endif

/** load a HDR file into an ShImage
  */
void HDRImage::loadHDR(const char filename[]) {
  FILE *f = fopen(filename, "rb");
  char line[2048], *g;
	int i,j,k;

  if (f == NULL) {
    std::cout<<"file not found"<<std::endl;
    return;
  }
	
	// read the "header"
	while ((g = fgets(line, sizeof(line), f)) && (strlen(g) > 1)); // skip first lines until an empty line is found
  fgets(line, sizeof(line), f);
  if ((sscanf(line, "+X %d +Y %d\n", &m_height, &m_width) != 2) && // get size infos
      (sscanf(line, "-Y %d +X %d\n", &m_height, &m_width) != 2)) {
    std::cout<<"Bad header: "<<line<<std::endl;
    return;
  }
	m_elements = 4;

	ShImage img(m_width, m_height, 4); // create ShImage with the appropriate size
	
	// read data in the file
  for (j=0 ; j<m_height ; j++) {
		int p;
  	p = getc(f);
  	if (p != 2 || p == EOF) {
			break;
		}
 		getc(f);	// ignore next character
	  if (((getc(f) << 8) | getc(f)) != m_width) { // another check on the size...
	    std::cout<<"error: length mismatch"<<std::endl;
			break;
		}
		int code, val;
	  for (k = 0 ; k<4 ; k++) { // R-G-B and an exponent value
	    for (i = 0 ; i<m_width ;) { // data are saved line by line
	      if ((code = getc(f)) == EOF) {
					break;
				}
	      if (code > 128) {	// loop over the same value
					code &= 127;
					val = getc(f);
					while (code--)
				 		img(i++,j,k) = val;
      	}
				else
					while (code--)
					  img(i++,j,k) = getc(f);
	    }	
		}
  }
  fclose(f);

	// compute the data by using the exponent value stored in the alpha channel
	for(i=0 ; i<m_width ; i++) {
		for(j=0 ; j<m_height ; j++)	{
			double v = ldexp(1.0, (int)(img(i,j,3) - 136));
			img(i,j,0) = img(i,j,0) * v;
			img(i,j,1) = img(i,j,1) * v;
			img(i,j,2) = img(i,j,2) * v;
			img(i,j,3) = 1.0;		
		}
	}
	m_image = img;
}

/** write the data of an ShImage to a HDR file
  */
void HDRImage::saveHDR(const char filename[]) {
  FILE *f = fopen(filename, "wb");
  
	if (f == NULL) {
    std::cout<<"error when creating/opening file"<<std::endl;
    return;
  }

	// convert from the image values to the values using a mantisse stored in the alpha channel
	int e;
	for(int i=0 ; i<m_width ; i++) {
		for(int j=0 ; j<m_height ; j++)	{
			float	d = m_image(i,j,0) > m_image(i,j,1) ? m_image(i,j,0) : m_image(i,j,1);
			if (m_image(i,j,2) > d)
				d = m_image(i,j,2);
			if (d <= 1e-32)
				m_image(i,j,0)=m_image(i,j,1)=m_image(i,j,2)=m_image(i,j,3)=0.0;
			else {
				d = frexp(d, &e) * 256 / d;
				m_image(i,j,0) = m_image(i,j,0)*d;
				m_image(i,j,1) = m_image(i,j,1)*d;
				m_image(i,j,2) = m_image(i,j,2)*d;
				m_image(i,j,3) = e + 128;
			}
		}
	}
		
	// write header
	fprintf(f,"#?RADIANCE\n");
	fprintf(f,"#written with Sh\n");
	fprintf(f,"FORMAT=32-bit_rle_rgbe\n");
	fprintf(f,"EXPOSURE=%25.13f\n",1.0);
	fprintf(f,"\n");
	fprintf(f,"-Y %d +X %d\n",m_height, m_width);

	// write the data
	int cnt = 1;
	int c2, beg;
	for (int j=0 ; j<m_height ; j++) {
		putc(2, f);
		putc(2, f);
		putc(m_width>>8, f);
		putc(m_width&255, f);
		for (int k = 0; k < 4; k++) {
	  	for (int i = 0; i < m_width; i += cnt) {	// find next run
				for (beg = i; beg < m_width; beg += cnt) {
					for (cnt = 1; cnt < 127 && beg+cnt < m_width && m_image(beg+cnt,j,k) == m_image(beg,j,k); cnt++); {
						if (cnt >= 4) {
							break;			// long enough
						}
					}
				}
				if (beg-i > 1 && beg-i < 4) {
					c2 = i+1;
				  while (m_image(c2++,j,k) == m_image(i,j,k)) {
						if (c2 == beg) {	// short run
					    putc(128+beg-i, f);
					    putc((int)m_image(i,j,k), f);
					    i = beg;
					    break;
						}
					}
				}
				while (i < beg) {		// write out non-run
					if ((c2 = beg-i) > 128)
						c2 = 128;
		  		putc(c2, f);
				 	while (c2--)
						putc((int)m_image(i++,j,k), f);
				}
				if (cnt >= 4) {		// write out run
				    putc(128+cnt, f);
				    putc((int)m_image(beg,j,k), f);
				}
				else
				  cnt = 0;
			}
		}
	}
	fclose(f);
}


