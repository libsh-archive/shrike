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
#include <cmath>
#include <sh/sh.hpp>
#include <HDRImage.hpp>

using namespace std;
using namespace SH;

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    cout << "Usage:" << endl;
    cout << endl;
    cout << " shsparse s <block size> <R> <G> <B> <A> <png file> : create a sparse image with RGBA as the default value" << endl;
    cout << " shsparse u <data file> <map file> : recompose an image" << endl;
    exit(1);
  }
  string type(argv[1]);
	if(type == "s") {
		int blocksize = atoi(argv[2]);
		float R = atol(argv[3]);
		float G = atol(argv[4]);
		float B = atol(argv[5]);
		float A = atol(argv[6]);
		string inFileName(argv[7]);
 		string mapFileName = inFileName.substr(0,inFileName.size() - 4) + "_map.hdr";
 		string dataFileName = inFileName.substr(0,inFileName.size() - 4) + "_data.hdr";
  	ShImage inputImage;
		HDRImage map, data;
  	load_PNG(inputImage, inFileName);
		int elements = inputImage.elements();

		// compute the number of blocks
		int block_horiz = inputImage.width()/blocksize;
		int block_vert = inputImage.height()/blocksize;
	
		map = HDRImage(block_horiz, block_vert, elements);
		for(int i=0 ; i<map.width() ; i++) {
			for(int j=0 ; j<map.height() ; j++) {
				for(int k=0 ; k<map.elements() ; k++) {
					map(i,j,k) = 0.0;
				}
			}
		}
		map(0,0,2) = (float)blocksize;
		data = HDRImage(blocksize, blocksize, elements);
		
		// create the background block
		for(int x=0 ; x<blocksize ; x++) {
			for(int y=0 ; y<blocksize ; y++) {
				data(x,y,0) = R;
				data(x,y,1) = G;
				data(x,y,2) = B;
				if(elements==4)
					data(x,y,3) = A;
			}
		}
			// check for different blocks
		int xblock = 1, yblock = 1; // the position of the end of the last block
		bool newblock = false;
		for(int j=0 ; j<block_vert ; j++) {
			for(int i=0 ; i<block_horiz ; i++) {
				newblock = true;
				for(int xoffset=0 ; xoffset<xblock ; xoffset++) {
					for(int yoffset=0 ; yoffset<yblock ; yoffset++) {
						float difference = 0.0; // initialization
						for(int x=0 ; x<blocksize ; x++) {
							for(int y=0 ; y<blocksize ; y++) {
								for(int z=0 ; z<elements ; z++) {
									difference += abs(inputImage(i*blocksize+x,j*blocksize+y,z) - data(xoffset*blocksize+x,yoffset*blocksize+y,z));
								}
							}
						}
						if(difference < blocksize) {
							newblock = false;
							map(i,j,0) = (float)(xoffset*blocksize);
							map(i,j,1) = (float)(yoffset*blocksize);
						}
					}
				}
				if(newblock) {
					if(xblock*blocksize == data.width() && yblock*blocksize == data.height()) {
						HDRImage tmp;
						if(xblock > yblock) {
							tmp = HDRImage(data.width(), data.height()+2*blocksize, elements);
							xblock=1;
							yblock++;
						}
						else {
							if(xblock == 1 && yblock == 1) {
								tmp = HDRImage(data.width()+blocksize, data.height()+blocksize, elements);
								xblock++;
							}
							else {
								tmp = HDRImage(data.width()+2*blocksize, data.height(), elements);
								xblock++;
								yblock=1;
							}
						}
						for(int x=0 ; x<data.width() ; x++) {
							for(int y=0 ; y<data.height() ; y++) {
								for(int z=0 ; z<elements ; z++) {
									tmp(x,y,z) = data(x,y,z);
								}
							}
						}
						data = tmp;
					}
					else {
						if(xblock%2 == (yblock+1)%2 && xblock%2==0) {
							xblock--;
							yblock++;
						}
						else {
							if(xblock%2 == yblock%2 && xblock%2 == 0) {
								if(yblock > xblock) {
									xblock++;
									yblock--;
								}
								else {
									xblock--;
									yblock++;
								}
							}
							else {
								xblock++;
							}
						}
					}
					map(i,j,0) = (float)(xblock-1)*blocksize;
					map(i,j,1) = (float)(yblock-1)*blocksize;
					for(int x=0 ; x<blocksize ; x++) {
						for(int y=0 ; y<blocksize ; y++) {
							for(int z=0 ; z<elements ; z++) {
								data(x+(xblock-1)*blocksize,y+(yblock-1)*blocksize,z) = inputImage(i*blocksize+x,j*blocksize+y,z);
							}
						}
					}
				}
			}
		}
		map.saveHDR(mapFileName.c_str());
		data.saveHDR(dataFileName.c_str());
	}
	else if(type == "u") {
		string dataFileName(argv[2]);
		string mapFileName(argv[3]);
  	HDRImage map, data;
  	map.loadHDR(mapFileName.c_str());
  	data.loadHDR(dataFileName.c_str());
		int blocksize = (int)rint(map(0,0,2));
		HDRImage image(map.width()*blocksize,map.height()*blocksize,4);
		string imageFileName = mapFileName.substr(0,mapFileName.size() - 8) + "_unsparsed.hdr";
		for(int i=0 ; i<map.width() ; i++) {
			for(int j=0 ; j<map.height() ; j++) {
				for(int x=0 ; x<blocksize ; x++) {
					for(int y=0 ; y<blocksize ; y++) {
						for(int k=0 ; k<map.elements() ; k++) {
							image(i*blocksize+x,j*blocksize+y,k) = data((int)rint(map(i,j,0))+x,(int)rint(map(i,j,1))+y,k);
						}
					}
				}
			}
		}
		image.saveHDR(imageFileName.c_str());
	}
  return 0;
}
