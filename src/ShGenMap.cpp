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

using namespace std;

int main(int argc, char** argv)
{
  if (argc < 3) {
    cout << "Usage:" << endl;
    cout << endl;
    cout << " shgenmap n <png file> : Create normal map" << endl;
    cout << " shgenmap q <png file> : Create quaternion map using the same png file" << endl;
    cout << " shgenmap q <png file1> <png file2> : Create quaternion map using 2 different png files (1 for normal, 2 for tangent)" << endl;
    cout << " shgenmap h <png file> : Create horizon maps (2 files for 8 directions):" << endl;
    exit(1);
  }
  string type(argv[1]);
  if (type == "n")  {
    string inFileName(argv[2]);
    string outFileName = inFileName.substr(0,inFileName.size() - 4) + 
      ".normal.png";
    SH::ShImage inputImage;
    inputImage.loadPng(inFileName);
    (inputImage.getNormalImage()).savePng16(outFileName);
  }
  else if (type == "q") {
    string inFileName(argv[2]);
    SH::ShImage inputImage, inputImage2;
    string outFileName = inFileName.substr(0,inFileName.size() - 4) + 
      ".quaternion.png";
    inputImage.loadPng(inFileName);
    SH::ShImage normalImage = inputImage.getNormalImage();
    SH::ShImage normalImage2;
    if (argc > 3) {
      inputImage2.loadPng(string(argv[3]));
      normalImage2 = inputImage2.getNormalImage();
    }
    int w = inputImage.width();
    int h = inputImage.height();
    int w2 = inputImage2.width();
    int h2 = inputImage2.height();
    SH::ShImage outputImage(w, h, 4);
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        SH::ShVector3f normal(2*normalImage(j, i, 0) - 1, 
			      2*normalImage(j, i, 1) - 1, 
			      2*normalImage(j, i, 2) - 1);
        SH::ShVector3f tan1 = normal;
        tan1(2) = 0;
        if (argc > 3) {
          int j2 = int((float(w2)/float(w))*j);
          int i2 = int((float(h2)/float(h))*i);
          
          tan1 = SH::ShVector3f(2*normalImage2(j2, i2, 0) - 1, 
				2*normalImage2(j2, i2, 1) - 1, 
				2*normalImage2(j2, i2, 2) - 1);
          tan1(2) = 0;
        }
        SH::ShAttrib1f norm = dot(tan1, tan1);
        float val;
        norm.getValues(&val);
        if (val < 0.000001) {
          outputImage(j, i, 0) = 1;
          outputImage(j, i, 1) = 0.5;
          outputImage(j, i, 2) = 0.5;
          outputImage(j, i, 3) = 0.5;
        } else {
          tan1 = normalize(cross(cross(tan1, normal), normal));
          SH::ShVector3f tan2 = cross(normal, tan1);
          SH::ShMatrix4x4f rot;
          rot[0](0) = tan1(0);
          rot[1](0) = tan1(1);
          rot[2](0) = tan1(2);
          rot[0](1) = tan2(0);
          rot[1](1) = tan2(1);
          rot[2](1) = tan2(2);
          rot[0](2) = normal(0);
          rot[1](2) = normal(1);
          rot[2](2) = normal(2);
          SH::ShQuaternionf frame(rot);
          frame.normalize();
          float vals[4];
          frame.getVector().getValues(vals);
          outputImage(j, i, 0) = vals[0]/2 + 0.5;
          outputImage(j, i, 1) = vals[1]/2 + 0.5;
          outputImage(j, i, 2) = vals[2]/2 + 0.5;
          outputImage(j, i, 3) = vals[3]/2 + 0.5;
        }
      }
    }
    outputImage.savePng16(outFileName);
  }
  else if (type == "h") {
    string inFileName(argv[2]);
    string outFileName1 = inFileName.substr(0,inFileName.size() - 4) +  "_horizon1.png";
    string outFileName2 = inFileName.substr(0,inFileName.size() - 4) +  "_horizon2.png";
		
    SH::ShImage inputImage;
    inputImage.loadPng(inFileName);
    SH::ShImage outputImage1(inputImage.width(), inputImage.height(), 4);
    SH::ShImage outputImage2(inputImage.width(), inputImage.height(), 4);

    int size_bump = 10;
    // clear the images
    for(int i=0 ; i<inputImage.width() ; i++)
      for(int j=0; j<inputImage.height() ; j++)
	for(int u=0 ; u<4 ; u++) {
	  outputImage1(i, j, u) = 1.0;
	  outputImage2(i, j, u) = 1.0;
	}
    // compute angle and direction
    for(int i=0 ; i<inputImage.width() ; i++) {
      for(int j=0; j<inputImage.height() ; j++) {
	float angle = 0.0;
	float bump;
	int u,v;
	u=i;
	// search the maximum angle for all directions
	while(u>0) {
	  u--;
	  bump = size_bump*(inputImage(u,j,0)-inputImage(i,j,0));
	  angle = (i-u) / sqrt((i-u)*(i-u)+bump*bump);
	  if(angle < outputImage1(i, j, 0) && bump>0)
	    outputImage1(i, j, 0) = angle;
	}
	angle = 0.0;
	v = j;
	while(v>0) {
	  v--;
	  bump = size_bump*(inputImage(i,v,0)-inputImage(i,j,0));
	  angle = (j-v) / sqrt((j-v)*(j-v)+bump*bump);
	  if(angle < outputImage1(i, j, 1) && bump>0)
	    outputImage1(i, j, 1) = angle;
      	}
	angle = 0.0;
	u = i;
	while(u<inputImage.width()) {
	  u++;
	  bump = size_bump*(inputImage(u, j, 0)-inputImage(i, j, 0));
	  angle = (u-i) / sqrt((u-i)*(u-i)+bump*bump);
	  if(angle < outputImage1(i, j, 2) && bump>0)
	    outputImage1(i, j, 2) = angle;
	}
	angle = 0.0;
	v = j;
      	while(v<inputImage.height()) {
	  v++;
	  bump = size_bump*(inputImage(i, v, 0)-inputImage(i, j, 0));
	  angle = (v-j) / sqrt((v-j)*(v-j)+bump*bump);
	  if(angle < outputImage1(i, j, 3) && bump>0)
	    outputImage1(i, j, 3) = angle;
	}
	angle = 0.0;
	u = i;v = j;
	while(u>0 && v>0) {
	  u--;
	  v--;
	  bump = size_bump*(inputImage(u, v, 0)-inputImage(i, j, 0));
	  angle = sqrt((float)((u-i)*(u-i)+(j-v)*(j-v))) / sqrt((i-u)*(i-u)+(j-v)*(j-v)+bump*bump);
	  if(angle < outputImage2(i, j, 0) && bump>0)
	    outputImage2(i, j, 0) = angle;
	}
	u = i;v = j;
	while(u<inputImage.width() && v>0) {
	  u++;
	  v--;
	  bump = size_bump*(inputImage(u, v, 0)-inputImage(i, j, 0));
	  angle = sqrt((float)((u-i)*(u-i)+(j-v)*(j-v))) / sqrt((u-i)*(u-i)+(j-v)*(j-v)+bump*bump);
	  if(angle < outputImage2(i, j, 1) && bump>0)
	    outputImage2(i, j, 1) = angle;
	}
      	u = i;v = j;
      	while(u<inputImage.width() && v<inputImage.height()) {
	  u++;
	  v++;
	  bump = size_bump*(inputImage(u, v, 0)-inputImage(i, j, 0));
	  angle = sqrt((float)((u-i)*(u-i)+(j-v)*(j-v))) / sqrt((u-i)*(u-i)+(j-v)*(j-v)+bump*bump);
	  if(angle < outputImage2(i, j, 2) && bump>0)
	    outputImage2(i, j, 2) = angle;
      	}
	u = i;v = j;
	while(u>0 && v<inputImage.height()) {
	  u--;
	  v++;
	  bump = size_bump*(inputImage(u, v, 0)-inputImage(i, j, 0));
	  angle = sqrt((float)((u-i)*(u-i)+(j-v)*(j-v))) / sqrt((u-i)*(u-i)+(j-v)*(j-v)+bump*bump);
	  if(angle < outputImage2(i, j, 3) && bump>0)
	    outputImage2(i, j, 3) = angle;
      	}
      }
    }
		
    outputImage1.savePng(outFileName1);
    outputImage2.savePng(outFileName2);
  }
  return 0;
}
