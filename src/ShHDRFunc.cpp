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
#include <fstream>
#include <string>
#include <math.h>
#include <sh/sh.hpp>
#include "HDRImage.hpp"

using namespace SH;
using namespace std;

HDRImage image, diffuse, specular;
int width, height, elements;
int exponent;

// Cubemap generation
HDRImage GenCubeMap(string inFileName);

// SBRDF generation
void VecFromCubeMap(float *x, float *y, float xl, float yl, float zl);
void computeDiffuse(int i, int j, float xn, float yn, float zn);
void computeSpecular(int i, int j, float xn, float yn, float zn);



int main(int argc, char **argv) {
  if(argc < 2) {
    cout<<"format: shHDRfunc cubemap [hdr file] : create a cubemap from a probe image"<<endl;
   	cout<<"format: shHDRfunc SBRDF diffuse [hdr cubemap file] : create the diffuse map"<<endl;
		cout<<"format: shHDRfunc SBRDF specular exponent [hdr cubemap file] : create the specular map"<<endl;
    cout<<"format: shHDRfunc png [hdr file] : convert the hdr file to a png one"<<endl;
    cout<<"format: shHDRfunc panoramic [hdr file] : create a panoramic view of a probe image"<<endl;
    return -1;
  }
  string option(argv[1]);
	if(option == "cubemap") {
    if(argc != 3) {
      cout<<"format: shHDRfunc cubemap [hdr file] : create a cubemap from a probe image"<<endl;
      return -1;
    }
    string inFileName(argv[2]);
   	string outFileName = inFileName.substr(0,inFileName.size() - 4) +  "_cubemap.hdr";
    HDRImage cubemap = GenCubeMap(inFileName);
    cubemap.saveHDR(outFileName.c_str());
  }
  if(option == "SBRDF") {
    if(argc < 3) {
   	  cout<<"format: shHDRfunc SBRDF diffuse [hdr cubemap file] : create the diffuse map"<<endl;
		  cout<<"format: shHDRfunc SBRDF specular exponent [hdr cubemap file] : create the specular map"<<endl;
      return -1;
    }
    string model(argv[2]);
    if(model == "diffuse") {
      if(argc != 4) {
   	    cout<<"format: shHDRfunc SBRDF diffuse [hdr cubemap file] : create the diffuse map"<<endl;
        return -1;
      }
      string inFileName(argv[3]);
     	image.loadHDR(inFileName.c_str());
    	width = image.width()/2;
    	height = image.height()/3;
		  string diffuseFileName = inFileName.substr(0,inFileName.size() - 4) +  "_SBRDF_diffuse.hdr";
  		diffuse = HDRImage(image.width(), image.height(), image.elements());

      for(int i=0 ; i<image.width() ; i++) {
  			for(int j=0 ; j<image.height() ; j++) {
  				for(int k=0 ; k<image.elements() ; k++) {
  					diffuse(i,j,k) = 0.0;
  				}
  			}
  		}
		
		  for(int i=0 ; i<width ; i++) {
			  for(int j=0 ; j<height ; j++) {
			  	// compute a normal vector and normalize it
				  float xn = (float)i/width-0.5;			
				  float yn = (float)j/height-0.5;
				  float zn = 0.5;
				  computeDiffuse(i, j, xn, yn, zn);
				  zn = -0.5;
				  computeDiffuse(i+width, j, xn, yn, zn);
				  zn = xn;
				  xn = 0.5;
				  computeDiffuse(i, j+height, xn, yn, zn);
				  xn = -0.5;
				  computeDiffuse(i+width, j+height, xn, yn, zn);
				  xn = zn;
				  zn = yn;
				  yn = 0.5;
				  computeDiffuse(i, j+2*height, xn, yn, zn);
				  yn = -0.5;
				  computeDiffuse(i+width, j+2*height, xn, yn, zn);
			  }
	  	}
		  diffuse.saveHDR(diffuseFileName.c_str());
    }
    if(model == "specular") {
      if(argc != 5) {
   	    cout<<"format: shHDRfunc SBRDF specular exponent [hdr cubemap file] : create the specular map"<<endl;
        return -1;
      }
      exponent = atoi(argv[3]);
      string inFileName(argv[4]);
     	image.loadHDR(inFileName.c_str());
    	width = image.width()/2;
    	height = image.height()/3;
  		string specularFileName = inFileName.substr(0,inFileName.size() - 4) +  "_SBRDF_specular_" + argv[3] +".hdr";
  		specular = HDRImage(image.width(), image.height(), image.elements());

		  for(int i=0 ; i<image.width() ; i++) {
			  for(int j=0 ; j<image.height() ; j++) {
				  for(int k=0 ; k<image.elements() ; k++) {
					  specular(i,j,k) = 0.0;
				  }
			  }
		  }
		
		  for(int i=0 ; i<width ; i++) {
			  for(int j=0 ; j<height ; j++) {
				  // initialize the values to 0	
				  float xn = (float)i/width-0.5;
				  float yn = (float)j/height-0.5;
				  float zn = 0.5;
				  computeSpecular(i, j, xn, yn, zn);
				  zn = -0.5;
				  computeSpecular(i+width, j, xn, yn, zn);
				  zn = xn;
				  xn = 0.5;
				  computeSpecular(i, j+height, xn, yn, zn);
				  xn = -0.5;
				  computeSpecular(i+width, j+height, xn, yn, zn);
				  xn = zn;
				  zn = yn;
				  yn = 0.5;
				  computeSpecular(i, j+2*height, xn, yn, zn);
				  yn = -0.5;
				  computeSpecular(i+width, j+2*height, xn, yn, zn);
			  }
		  }
		  specular.saveHDR(specularFileName.c_str());
    }
  }
	if(option == "png") {
    if(argc != 3) {
      cout<<"format: shHDRfunc png [hdr file] : convert the hdr file to the png format"<<endl;
      return -1;
    }
    string inFileName(argv[2]);
   	string outFileName = inFileName.substr(0,inFileName.size() - 3) +  "png";
  	image.loadHDR(inFileName.c_str());
    width = image.width();
    height = image.height();
    for(int i=0 ; i<width ; i++) {
      for(int j=0 ; j<height ; j++) {
        image(i,j,3) = 1.0;
        for(int k=0 ; k<3 ; k++) {
          image(i,j,k) *= 55.5555098;
          if(image(i,j,k) > 1.0)
            image(i,j,k) = 1.0 + log((image(i,j,k)-1.0) * 0.184874 +1.0) / 0.184874; 
          image(i,j,k) = pow(image(i,j,k),(float)0.454545455);
          image(i,j,k) *= 0.285714286;
          if(image(i,j,k) > 1.0)
            image(i,j,k) = 1.0;
          if(image(i,j,k) < 0.0)
            image(i,j,k) = 0.0;
        }
      }
    }
  	image.savePng(outFileName);
  }
	if(option == "panoramic") {
    if(argc != 3) {
      cout<<"format: shHDRfunc panoramic [hdr file] : create a panoramic view of a probe image"<<endl;
      return -1;
    }
    string inFileName(argv[2]);
 	  string outFileName = inFileName.substr(0,inFileName.size() - 4) +  "_panoramic.png";
    image = GenCubeMap(inFileName);
    width = image.width();
    height = image.height();
    // tonemapping
    for(int i=0 ; i<width ; i++) {
      for(int j=0 ; j<height ; j++) {
        image(i,j,3) = 1.0;
        for(int k=0 ; k<3 ; k++) {
          image(i,j,k) *= 55.5555098;
          if(image(i,j,k) > 1.0)
            image(i,j,k) = 1.0 + log((image(i,j,k)-1.0) * 0.184874 +1.0) / 0.184874; 
          image(i,j,k) = pow(image(i,j,k),(float)0.454545455);
          image(i,j,k) *= 0.285714286;
          if(image(i,j,k) > 1.0)
            image(i,j,k) = 1.0;
          if(image(i,j,k) < 0.0)
            image(i,j,k) = 0.0;
        }
      }
    }
    width = width/2;
    height = height/3;
    ShImage panoramic(2*width, height/2, 3);

	  for(int i=0 ; i<width/2 ; i++) {
		  for(int j=0 ; j<height/2 ; j++) {
			  for(int k=0 ; k<3 ; k++) {
          panoramic(i,j,k) = image(i+width,j+height,k);
          panoramic(i+width,j,k) = image(i,j,k);
          panoramic(i+2*width,j,k) = image(width-i-1,j+height,k);
          panoramic(i+3*width,j,k) = image(2*width-i-1,j,k);
        }
      }
    }
    panoramic.savePng(outFileName);
  }
  return 0;
}


HDRImage GenCubeMap(string inFileName) {
	image.loadHDR(inFileName.c_str());
  width = image.width();
  height = image.height();

	HDRImage cubemap(width+4,3*height/2+6,4);
	ShImage cur1(width,height,4); // used for the reduction of images
	ShImage cur2(width,height,4);

	for(int i=0 ; i<width ; i++) {
		for(int j=0 ; j<height ; j++) {
			float x = (float)i/width-0.5;
			float y = (float)j/height-0.5;
			float z = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y)); // change from spheric coordinates to planar coordinates
			int u = (int)((x*r + 1.0) * width/2);
			int v = (int)((y*r + 1.0) * height/2);
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = image(u,v,k); // save the info to a temporary image
			}
			r = acos(-z)/(M_PI*sqrt(x*x+y*y));
			u = (int)((x*r + 1.0) * width/2); 
			v = (int)((y*r + 1.0) * height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = image(u,v,k); // save the other side
			}
		}
	}
	
	// shrink the 2 images with a bilinear interpolation and save them in the cubemap image
	for(int i=0 ; i<width/2 ; i++) {
		for(int j=0 ; j<height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = std::min(2*(i+1),width-1);
				int v = std::min(2*(j+1),height-1);
				cubemap(i+1,j+1,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				cubemap(i+width/2+3,j+1,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}

	// 2 other faces
	for(int i=0 ; i<width ; i++) {
		for(int j=0 ; j<height ; j++) {
			float z = (float)i/width-0.5;
			float y = (float)j/height-0.5;
			float x = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y));
			int u = (int)((x*r + 1.0) * width/2); 
			int v = (int)((y*r + 1.0) * height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = image(u,v,k);
			}
			u = (int)((-x*r + 1.0) * width/2); 
			v = (int)((y*r + 1.0) * height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = image(u,v,k);
			}
		}
	}
	
	// shrink the 2 new faces
	for(int i=0 ; i<width/2 ; i++) {
		for(int j=0 ; j<height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = std::min(2*(i+1),width-1);
				int v = std::min(2*(j+1),height-1);
				cubemap(i+1,j+height/2+3,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				cubemap(i+width/2+3,j+height/2+3,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}
	
	// the 2 last faces
	for(int i=0 ; i<width ; i++) {
		for(int j=0 ; j<height ; j++) {
			float x = (float)i/width-0.5;
			float z = (float)j/height-0.5;
			float y = 0.5;
			float norm = sqrt(x*x+y*y+z*z);
			x /= norm;
			y /= norm;
			z /= norm;
			float r = acos(z)/(M_PI*sqrt(x*x+y*y));
			int u = (int)((x*r + 1.0) * width/2); 
			int v = (int)((y*r + 1.0) * height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur1(i,j,k) = image(u,v,k);
			}
			u = (int)((x*r + 1.0) * width/2); 
			v = (int)((-y*r + 1.0) * height/2); 	
			for(int k=0 ; k<4 ; k++) {
				cur2(i,j,k) = image(u,v,k);
			}
		}
	}
	
	// shrink
	for(int i=0 ; i<width/2 ; i++) {
		for(int j=0 ; j<height/2 ; j++) {
			for(int k=0 ; k<4 ; k++) {
				int u = std::min(2*(i+1),width-1);
				int v = std::min(2*(j+1),height-1);
				cubemap(i+1,j+height+5,k) = 0.25*(cur1(2*i,2*j,k)+cur1(u,2*j,k)+cur1(2*i,v,k)+cur1(u,v,k));
				cubemap(i+width/2+3,j+height+5,k) = 0.25*(cur2(2*i,2*j,k)+cur2(u,2*j,k)+cur2(2*i,v,k)+cur2(u,v,k));
			}
		}
	}

	// compute the intersections between the faces, 4 lines by face
	for(int j=0 ; j<height/2 ; j++) {
		for(int k=0 ; k<4 ; k++) {
			cubemap(0,j+1,k) = cubemap(width+2,j+height/2+3,k);
			cubemap(width+3,j+height/2+3,k) = cubemap(1,j+1,k);
			
			cubemap(width/2+1,j+1,k) = cubemap(width/2,j+height/2+3,k);
			cubemap(width/2+1,j+height/2+3,k) = cubemap(width/2,j+1,k);

			cubemap(width/2+2,j+1,k) = cubemap(width/2+3,j+height/2+3,k);
			cubemap(width/2+2,j+height/2+3,k) = cubemap(width/2+3,j+1,k); 

			cubemap(width+3,j+1,k) = cubemap(1,j+height/2+3,k);
			cubemap(0,j+height/2+3,k) = cubemap(width+2,j+1,k);
		
			cubemap(width/2+1,j+height+5,k) = cubemap(j+1,height+2,k);			
			cubemap(j+1,height+3,k) = cubemap(width/2,j+height+5,k);
			
			cubemap(0,j+height+5,k) = cubemap(j+width/2+3,height+2,k);			
			cubemap(j+width/2+3,height+3,k) = cubemap(1,j+height+5,k);
		}
	}

	for(int i=0 ; i<width/2 ; i++) {
		for(int k=0 ; k<4 ; k++) {
			cubemap(i+1,0,k) = cubemap(i+width/2+3,3*height/2+4,k);
			cubemap(i+width/2+3,3*height/2+5,k) = cubemap(i+1,1,k);
			
			cubemap(i+1,height/2+1,k) = cubemap(i+1,3*height/2+4,k);
			cubemap(i+1,3*height/2+5,k) = cubemap(i+1,height/2,k);
		
			cubemap(i+width/2+3,0,k) = cubemap(i+width/2+3,height+5,k);
			cubemap(i+width/2+3,height+4,k) = cubemap(i+width/2+3,1,k);
			
			cubemap(i+width/2+3,height/2+1,k) = cubemap(i+1,height+5,k);
			cubemap(i+1,height+4,k) = cubemap(i+width/2+3,height/2,k);
			
			cubemap(i+1,height/2+2,k) = cubemap(width+2,i+height+5,k);
			cubemap(width+3,i+height+5,k) = cubemap(i+1,height/2+3,k);

			cubemap(i+width/2+3,height/2+2,k) = cubemap(width/2+3,i+height+5,k);
			cubemap(width/2+2,i+height+5,k) = cubemap(i+width/2+3,height/2+3,k);
		}
	}

	// compute the pixels at the corner of the faces, 4 pixels by face 
	// the value is the average of the 3 adjacent pixel values
	for(int k=0 ; k<4 ; k++) {
		cubemap(0,0,k) = 0.333333*(cubemap(1,0,k) + cubemap(0,1,k) + cubemap(1,1,k));
		cubemap(width/2+1,0,k) = 0.333333*(cubemap(width/2,0,k) +
																		 cubemap(width/2+1,1,k) +
																		 cubemap(width/2,1,k));
		cubemap(width/2+2,0,k) = 0.333333*(cubemap(width/2+3,0,k) +
																		 cubemap(width/2+2,1,k) +
																		 cubemap(width/2+3,1,k));
		cubemap(width+3,0,k) = 0.333333*(cubemap(width+2,0,k) +
																	 cubemap(width+2,1,k) +
																	 cubemap(width+3,1,k));
		
		cubemap(0,height/2+1,k) = 0.333333*(cubemap(0,height/2,k) +
																			cubemap(1,height/2,k) +
																			cubemap(1,height/2+1,k));
		cubemap(width/2+1,height/2+1,k) = 0.333333*(cubemap(width/2+1,height/2,k) +
																								cubemap(width/2,height/2,k) +	
																								cubemap(width/2,height/2+1,k));
		cubemap(width/2+2,height/2+1,k) = 0.333333*(cubemap(width/2+2,height/2,k) +
																								cubemap(width/2+3,height/2+1,k) +
																								cubemap(width/2+3,height/2,k));
		cubemap(width+3,height/2+1,k) = 0.333333*(cubemap(width+3,height/2,k) +
																							cubemap(width+2,height/2+1,k) +
																							cubemap(width+2,height/2,k));
		
		cubemap(0,height/2+2,k) = 0.333333*(cubemap(0,height/2+3,k) +
																			cubemap(1,height/2+2,k) +
																			cubemap(1,height/2+3,k));
		cubemap(width/2+1,height/2+2,k) = 0.333333*(cubemap(width/2+1,height/2+3,k) +
																								cubemap(width/2,height/2+3,k) +
																								cubemap(width/2,height/2+2,k));
		cubemap(width/2+2,height/2+2,k) = 0.333333*(cubemap(width/2+2,height/2+3,k) +
																								cubemap(width/2+3,height/2+3,k) +
																								cubemap(width/2+3,height/2+2,k));
		cubemap(width+3,height/2+2,k) = 0.333333*(cubemap(width+3,height/2+3,k) +
																							cubemap(width+2,height/2+2,k) +
																							cubemap(width+3,height/2+3,k));

		cubemap(0,height+3,k) = 0.333333*(cubemap(0,height+2,k) +
																		cubemap(1,height+2,k) +
																		cubemap(1,height+3,k));
		cubemap(width/2+1,height+3,k) = 0.333333*(cubemap(width/2+1,height+2,k) +
																							cubemap(width/2,height+2,k) +
																							cubemap(width/2,height/2+1,k));
		cubemap(width/2+2,height+3,k) = 0.333333*(cubemap(width/2+2,height+2,k) +
																							cubemap(width/2+3,height+3,k) +
																							cubemap(width/2+3,height+2,k));
		cubemap(width+3,height+3,k) = 0.333333*(cubemap(width+3,height+2,k) +
																						cubemap(width+2,height+3,k) +
																						cubemap(width+2,height+2,k));
		
		cubemap(0,height+4,k) = 0.333333*(cubemap(0,height+5,k) +
																		cubemap(1,height+5,k) +
																		cubemap(1,height+4,k));
		cubemap(width/2+1,height+4,k) = 0.333333*(cubemap(width/2+1,height+5,k) +
																							cubemap(width/2,height+5,k) +
																							cubemap(width/2,height+4,k));
		cubemap(width/2+2,height+4,k) = 0.333333*(cubemap(width/2+2,height+5,k) +
																							cubemap(width/2+3,height+5,k) +
																							cubemap(width/2+3,height+4,k));
		cubemap(width+3,height+4,k) = 0.333333*(cubemap(width+3,height+5,k) +
																						cubemap(width+2,height+4,k) +
																						cubemap(width+3,height+5,k));

		cubemap(0,3*height/2+5,k) = 0.333333*(cubemap(1,3*height/2+5,k) +
																				cubemap(0,3*height/2+4,k) +
																				cubemap(1,3*height/2+4,k));
		cubemap(width/2+1,3*height/2+5,k) = 0.333333*(cubemap(width/2,3*height/2+5,k) +
																									cubemap(width/2+1,3*height/2+4,k) +
																									cubemap(width/2,3*height/2+4,k));
		cubemap(width/2+2,3*height/2+5,k) = 0.333333*(cubemap(width/2+3,3*height/2+5,k) +
																									cubemap(width/2+2,3*height/2+4,k) +
																									cubemap(width/2+3,3*height/2+4,k));
		cubemap(width+3,3*height/2+5,k) = 0.333333*(cubemap(width+2,3*height/2+5,k) +
																								cubemap(width+2,3*height/2+4,k) +
																								cubemap(width+3,3*height/2+4,k));		
	}

  return cubemap;
}


void VecFromCubeMap(float *x, float *y, float xl, float yl, float zl) {
	float absx=fabs(xl), absy=fabs(yl), absz=fabs(zl);
	if(absx >= absy && absx >= absz) {
		*x = zl/absx + 1.0;
		if(xl < 0)
			*x += 2.0;
		*y = yl/absx + 3.0;
	}
	else if(absy >= absz) {
		*x = xl/absy + 1.0;
		if(yl < 0)
			*x += 2.0;
		*y = zl/absy + 5.0;
	}
	else {
		*x = xl/absz + 1.0;
		if(zl < 0)
			*x += 2.0;
		*y = yl/absz + 1.0;
	}
}

void computeDiffuse(int i, int j, float xn, float yn, float zn) {
	float norm = sqrt(xn*xn+yn*yn+zn*zn);
	xn /= norm;
	yn /= norm;
	zn /= norm;
	// counters initialization
	int cmpt = 0;
	for(int u=0 ; u<180 ; u=u+5) {
		for(int v=0 ; v<360 ; v=v+5) {
			// compute the light vector
			float xl = cos(u*M_PI/180) * cos(v*M_PI/180);
			float yl = sin(u*M_PI/180) * cos(v*M_PI/180);
			float zl = sin(v*M_PI/180);
			float dotprod = xn*xl + yn*yl + zn*zl;
			if(dotprod <= 0.0) // cancel negative dot-products
				continue;
			cmpt++;
			float x,y;
			VecFromCubeMap(&x, &y, xl, yl, zl); // compute the coordinates relative to the cube map
			x *= width/2;
			y *= height/2;
			x--;
			y--;
			for(int k=0 ; k<4 ; k++) { // add the effect of the light vector
				diffuse(i,j,k) += dotprod*image((int)x,(int)y,k);
			}
		}
	}							
	for(int k=0 ; k<4 ; k++) {
			diffuse(i,j,k) /= cmpt;
	}
}

void computeSpecular(int i, int j, float xn, float yn, float zn) {
	float norm = sqrt(xn*xn+yn*yn+zn*zn);
	xn /= norm;
	yn /= norm;
	zn /= norm;
	float absx=fabs(xn), absy=fabs(yn), absz=fabs(zn);
	float xdir=0.0, ydir=0.0, zdir = 0.0;
	if(absx >= absy && absx >= absz)
		xdir = xn/absx;
	else if(absy >= absz)
		ydir = yn/absy;
	else
		zdir = zn/absz;
	float WgtSum = 0.0;
	for(int u=0 ; u<180 ; u=u+2) {
		for(int v=0 ; v<360 ; v=v+2) {
			// compute the light vector
			float xl = cos(u*M_PI/180) * sin(v*M_PI/180);
			float yl = sin(u*M_PI/180) * sin(v*M_PI/180);
			float zl = cos(v*M_PI/180);
			if((xdir*xl + ydir*yl + zdir*zl) < 0.0)
				continue;
			float dotprod = xn*xl + yn*yl + zn*zl;
			if(dotprod <= 0.0) // cancel negative dot-products
				continue;
			float powdotprod = pow(dotprod,exponent);
			float x,y;
			VecFromCubeMap(&x, &y, xl, yl, zl); // compute the coordinates relative to the cube map
			x *= width/2;
			y *= height/2;
			x--;
			y--;
			WgtSum += powdotprod;
			for(int z=0 ; z<4 ; z++) { // add the effect of the light vector
				specular((int)i,(int)j,z) += powdotprod * image((int)x,(int)y,z);
			}
		}
	}
	for(int z=0 ; z<4 ; z++) {
		specular((int)i,(int)j,z) /= WgtSum;
	}
}






