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
#include "HDRCubeMap.hpp"

using namespace SH;

HDRImage image, diffuse, specular;
int width, height;
int exponent;

void VecFromCubeMap(float *x, float *y, float xl, float yl, float zl) {
	float absx=abs(xl), absy=abs(yl), absz=abs(zl);
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
	float absx=abs(xn), absy=abs(yn), absz=abs(zn);
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

int main(int argc, char **argv) {
	if(argc < 2) {
		cout<<"format: shgenSBRDF d [hdr cubemap file] : create the diffuse map"<<endl;
		cout<<"format: shgenSBRDF s [hdr cubemap file] n : create the specular map with n as exponent value"<<endl;
		return -1;
	}
  string type(argv[1]);
	string inFileName(argv[2]);
	image.loadHDR(inFileName.c_str());
	width = image.width()/2;
	height = image.height()/3;

	if(type == "d") {
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
			if(i%10 == 0)
				cout<<"i: "<<i<<endl;
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
	else if(type == "s") {
		exponent = atoi(argv[3]);
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
			if(i%10 == 0)
				cout<<"i: "<<i<<endl;;
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
	return 0;
}
