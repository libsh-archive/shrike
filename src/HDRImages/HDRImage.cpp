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
/*
#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfRgba.h>
#include <ImfArray.h>
#include <ImathBox.h>


using namespace Imath;
using namespace Imf;
*/
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

/*
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
	cout<<m_width<<" "<<m_height<<endl;
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
*/

// load a HDR file
void HDRImage::loadHDR(const char filename[]) {
  FILE *f = fopen(filename, "rb");
  char line[2048], *g;
	int i,j,k;

  if (f == NULL) {
    cout<<"file not found"<<endl;
    return;
  }
	
	// read the "header"
	while ((g = fgets(line, sizeof(line), f)) && (strlen(g) > 1)); // skip first lines until an empty line is found
  fgets(line, sizeof(line), f);
  if ((sscanf(line, "+X %d +Y %d\n", &m_height, &m_width) != 2) && // get size infos
      (sscanf(line, "-Y %d +X %d\n", &m_height, &m_width) != 2)) {
    cout<<"Bad header: "<<line<<endl;
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
	    cout<<"error: length mismatch"<<endl;
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
			double v = ldexp(1.0, (int)(img(i,j,3) - (128+8)));
			img(i,j,0) = (img(i,j,0)+0.5) * v;
			img(i,j,1) = (img(i,j,1)+0.5) * v;
			img(i,j,2) = (img(i,j,2)+0.5) * v;
			img(i,j,3) = 1.0;		
		}
	}
	m_image = img;
}

void HDRImage::saveHDR(const char filename[]) {
  FILE *f = fopen(filename, "wb");
  
	if (f == NULL) {
    cout<<"error when creating/opening file"<<endl;
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
				m_image(i,j,0) = m_image(i,j,0)*d - 0.5;
				m_image(i,j,1) = m_image(i,j,1)*d - 0.5;
				m_image(i,j,2) = m_image(i,j,2)*d - 0.5;
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

void HDRImage::CreateHDRCubeMap(const char filename[]) {
	ShImage tmp = m_image;
	ComputeCubeMap();
	m_image = m_cubemapimage;
	m_width = m_cubemapimage.width();
	m_height = m_cubemapimage.height();
	saveHDR(filename);
	m_image = tmp;
	m_width = m_image.width();
	m_height = m_image.height();
}

// create a cubemap texture
void HDRImage::ComputeCubeMap() {
	ShImage img(m_width+4,3*m_height/2+6,4);
	ShImage cur1(m_width,m_height,4); // used for the reduction of images
	ShImage cur2(m_width,m_height,4);

	for(int i=0 ; i<m_width ; i++) {
		for(int j=0 ; j<m_height ; j++) {
			float x = (float)i/m_width-0.5;
			float y = (float)j/m_height-0.5;
			float z = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y)); // change from spheric coordinates to planar coordinates
			int u = (int)((x*r + 1.0) * m_width/2);
			int v = (int)((y*r + 1.0) * m_height/2);
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = m_image(u,v,k); // save the info to a temporary image
			}
			r = acos(-z)/(M_PI*sqrt(x*x+y*y));
			u = (int)((x*r + 1.0) * m_width/2); 
			v = (int)((y*r + 1.0) * m_height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = m_image(u,v,k); // save the other side
			}
		}
	}
	
	// shrink the 2 images with a bilinear interpolation and save them in the cubemap image
	for(int i=0 ; i<m_width/2 ; i++) {
		for(int j=0 ; j<m_height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = min(2*(i+1),m_width-1);
				int v = min(2*(j+1),m_height-1);
				img(i+1,j+1,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				img(i+m_width/2+3,j+1,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}

	// 2 other faces
	for(int i=0 ; i<m_width ; i++) {
		for(int j=0 ; j<m_height ; j++) {
			float z = (float)i/m_width-0.5;
			float y = (float)j/m_height-0.5;
			float x = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y));
			int u = (int)((x*r + 1.0) * m_width/2); 
			int v = (int)((y*r + 1.0) * m_height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = m_image(u,v,k);
			}
			u = (int)((-x*r + 1.0) * m_width/2); 
			v = (int)((y*r + 1.0) * m_height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = m_image(u,v,k);
			}
		}
	}
	
	// shrink the 2 new faces
	for(int i=0 ; i<m_width/2 ; i++) {
		for(int j=0 ; j<m_height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = min(2*(i+1),m_width-1);
				int v = min(2*(j+1),m_height-1);
				img(i+1,j+m_height/2+3,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				img(i+m_width/2+3,j+m_height/2+3,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}
	
	// the 2 last faces
	for(int i=0 ; i<m_width ; i++) {
		for(int j=0 ; j<m_height ; j++) {
			float x = (float)i/m_width-0.5;
			float z = (float)j/m_height-0.5;
			float y = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y));
			int u = (int)((x*r + 1.0) * m_width/2); 
			int v = (int)((y*r + 1.0) * m_height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = m_image(u,v,k);
			}
			u = (int)((x*r + 1.0) * m_width/2); 
			v = (int)((-y*r + 1.0) * m_height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = m_image(u,v,k);
			}
		}
	}
	
	// shrink
	for(int i=0 ; i<m_width/2 ; i++) {
		for(int j=0 ; j<m_height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = min(2*(i+1),m_width-1);
				int v = min(2*(j+1),m_height-1);
				img(i+1,j+m_height+5,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				img(i+m_width/2+3,j+m_height+5,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}

	// compute the intersections between the faces, 4 lines by face
	for(int j=0 ; j<m_height/2 ; j++) {
		for(int k=0 ; k<4 ; k++) {
			img(0,j+1,k) = img(m_width+2,j+m_height/2+3,k);
			img(m_width+3,j+m_height/2+3,k) = img(1,j+1,k);
			
			img(m_width/2+1,j+1,k) = img(m_width/2,j+m_height/2+3,k);
			img(m_width/2+1,j+m_height/2+3,k) = img(m_width/2,j+1,k);

			img(m_width/2+2,j+1,k) = img(m_width/2+3,j+m_height/2+3,k);
			img(m_width/2+2,j+m_height/2+3,k) = img(m_width/2+3,j+1,k); 

			img(m_width+3,j+1,k) = img(1,j+m_height/2+3,k);
			img(0,j+m_height/2+3,k) = img(m_width+2,j+1,k);
		
			img(m_width/2+1,j+m_height+5,k) = img(j+1,m_height+2,k);			
			img(j+1,m_height+3,k) = img(m_width/2,j+m_height+5,k);
			
			img(0,j+m_height+5,k) = img(j+m_width/2+3,m_height+2,k);			
			img(j+m_width/2+3,m_height+3,k) = img(1,j+m_height+5,k);
		}
	}

	for(int i=0 ; i<m_width/2 ; i++) {
		for(int k=0 ; k<4 ; k++) {
			img(i+1,0,k) = img(i+m_width/2+3,3*m_height/2+4,k);
			img(i+m_width/2+3,3*m_height/2+5,k) = img(i+1,1,k);
			
			img(i+1,m_height/2+1,k) = img(i+1,3*m_height/2+4,k);
			img(i+1,3*m_height/2+5,k) = img(i+1,m_height/2,k);
		
			img(i+m_width/2+3,0,k) = img(i+m_width/2+3,m_height+5,k);
			img(i+m_width/2+3,m_height+4,k) = img(i+m_width/2+3,1,k);
			
			img(i+m_width/2+3,m_height/2+1,k) = img(i+1,m_height+5,k);
			img(i+1,m_height+4,k) = img(i+m_width/2+3,m_height/2,k);
			
			img(i+1,m_height/2+2,k) = img(m_width+2,i+m_height+5,k);
			img(m_width+3,i+m_height+5,k) = img(i+1,m_height/2+3,k);

			img(i+m_width/2+3,m_height/2+2,k) = img(m_width/2+3,i+m_height+5,k);
			img(m_width/2+2,i+m_height+5,k) = img(i+m_width/2+3,m_height/2+3,k);
		}
	}

	// compute the pixels at the corner of the faces, 4 pixels by face 
	// the value is the average of the 3 adjacent pixel values
	for(int k=0 ; k<4 ; k++) {
		img(0,0,k) = 0.333333*(img(1,0,k) + img(0,1,k) + img(1,1,k));
		img(m_width/2+1,0,k) = 0.333333*(img(m_width/2,0,k) +
																		 img(m_width/2+1,1,k) +
																		 img(m_width/2,1,k));
		img(m_width/2+2,0,k) = 0.333333*(img(m_width/2+3,0,k) +
																		 img(m_width/2+2,1,k) +
																		 img(m_width/2+3,1,k));
		img(m_width+3,0,k) = 0.333333*(img(m_width+2,0,k) +
																	 img(m_width+2,1,k) +
																	 img(m_width+3,1,k));
		
		img(0,m_height/2+1,k) = 0.333333*(img(0,m_height/2,k) +
																			img(1,m_height/2,k) +
																			img(1,m_height/2+1,k));
		img(m_width/2+1,m_height/2+1,k) = 0.333333*(img(m_width/2+1,m_height/2,k) +
																								img(m_width/2,m_height/2,k) +	
																								img(m_width/2,m_height/2+1,k));
		img(m_width/2+2,m_height/2+1,k) = 0.333333*(img(m_width/2+2,m_height/2,k) +
																								img(m_width/2+3,m_height/2+1,k) +
																								img(m_width/2+3,m_height/2,k));
		img(m_width+3,m_height/2+1,k) = 0.333333*(img(m_width+3,m_height/2,k) +
																							img(m_width+2,m_height/2+1,k) +
																							img(m_width+2,m_height/2,k));
		
		img(0,m_height/2+2,k) = 0.333333*(img(0,m_height/2+3,k) +
																			img(1,m_height/2+2,k) +
																			img(1,m_height/2+3,k));
		img(m_width/2+1,m_height/2+2,k) = 0.333333*(img(m_width/2+1,m_height/2+3,k) +
																								img(m_width/2,m_height/2+3,k) +
																								img(m_width/2,m_height/2+2,k));
		img(m_width/2+2,m_height/2+2,k) = 0.333333*(img(m_width/2+2,m_height/2+3,k) +
																								img(m_width/2+3,m_height/2+3,k) +
																								img(m_width/2+3,m_height/2+2,k));
		img(m_width+3,m_height/2+2,k) = 0.333333*(img(m_width+3,m_height/2+3,k) +
																							img(m_width+2,m_height/2+2,k) +
																							img(m_width+3,m_height/2+3,k));

		img(0,m_height+3,k) = 0.333333*(img(0,m_height+2,k) +
																		img(1,m_height+2,k) +
																		img(1,m_height+3,k));
		img(m_width/2+1,m_height+3,k) = 0.333333*(img(m_width/2+1,m_height+2,k) +
																							img(m_width/2,m_height+2,k) +
																							img(m_width/2,m_height/2+1,k));
		img(m_width/2+2,m_height+3,k) = 0.333333*(img(m_width/2+2,m_height+2,k) +
																							img(m_width/2+3,m_height+3,k) +
																							img(m_width/2+3,m_height+2,k));
		img(m_width+3,m_height+3,k) = 0.333333*(img(m_width+3,m_height+2,k) +
																						img(m_width+2,m_height+3,k) +
																						img(m_width+2,m_height+2,k));
		
		img(0,m_height+4,k) = 0.333333*(img(0,m_height+5,k) +
																		img(1,m_height+5,k) +
																		img(1,m_height+4,k));
		img(m_width/2+1,m_height+4,k) = 0.333333*(img(m_width/2+1,m_height+5,k) +
																							img(m_width/2,m_height+5,k) +
																							img(m_width/2,m_height+4,k));
		img(m_width/2+2,m_height+4,k) = 0.333333*(img(m_width/2+2,m_height+5,k) +
																							img(m_width/2+3,m_height+5,k) +
																							img(m_width/2+3,m_height+4,k));
		img(m_width+3,m_height+4,k) = 0.333333*(img(m_width+3,m_height+5,k) +
																						img(m_width+2,m_height+4,k) +
																						img(m_width+3,m_height+5,k));

		img(0,3*m_height/2+5,k) = 0.333333*(img(1,3*m_height/2+5,k) +
																				img(0,3*m_height/2+4,k) +
																				img(1,3*m_height/2+4,k));
		img(m_width/2+1,3*m_height/2+5,k) = 0.333333*(img(m_width/2,3*m_height/2+5,k) +
																									img(m_width/2+1,3*m_height/2+4,k) +
																									img(m_width/2,3*m_height/2+4,k));
		img(m_width/2+2,3*m_height/2+5,k) = 0.333333*(img(m_width/2+3,3*m_height/2+5,k) +
																									img(m_width/2+2,3*m_height/2+4,k) +
																									img(m_width/2+3,3*m_height/2+4,k));
		img(m_width+3,3*m_height/2+5,k) = 0.333333*(img(m_width+2,3*m_height/2+5,k) +
																								img(m_width+2,3*m_height/2+4,k) +
																								img(m_width+3,3*m_height/2+4,k));		
	}

	m_cubemapimage = img; // save the cube map to a specific image
}

