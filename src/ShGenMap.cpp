#include <iostream>
#include <fstream>
#include <string>
#include <sh/sh.hpp>

using namespace std;

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    cout << "Usage:" << endl;
    cout << endl;
    cout << " shgenmap n <png file> : Create normal map" << endl;
    cout << " shgenmap q <png file> : Create quaternion map using the same png file" << endl;
    cout << " shgenmap q <png file1> <png file2> : Create quaternion map using 2 different png files (1 for normal, 2 for tangent)" << endl;
    exit(1);
  }
  string type(argv[1]);
  if (type == "n") 
  {
    string inFileName(argv[2]);
    string outFileName = inFileName.substr(0,inFileName.size() - 4) + 
      ".normal.png";
    SH::ShImage inputImage;
    inputImage.loadPng(inFileName);
    (inputImage.getNormalImage()).savePng16(outFileName);
  }
  else if (type == "q") 
  {
    string inFileName(argv[2]);
    SH::ShImage inputImage, inputImage2;
    string outFileName = inFileName.substr(0,inFileName.size() - 4) + 
      ".quaternion.png";
    inputImage.loadPng(inFileName);
    SH::ShImage normalImage = inputImage.getNormalImage();
    SH::ShImage normalImage2;
    if (argc > 3) 
    {
      inputImage2.loadPng(string(argv[3]));
      normalImage2 = inputImage2.getNormalImage();
    }
    int w = inputImage.width();
    int h = inputImage.height();
    int w2 = inputImage2.width();
    int h2 = inputImage2.height();
    SH::ShImage outputImage(w, h, 4);
    for (int i = 0; i < h; i++) 
    {
      for (int j = 0; j < w; j++)
      {
        SH::ShVector3f normal(2*normalImage(j, i, 0) - 1, 
            2*normalImage(j, i, 1) - 1, 
            2*normalImage(j, i, 2) - 1);
        SH::ShVector3f tan1 = normal;
        tan1(2) = 0;
        if (argc > 3) 
        {
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
        if (val < 0.000001) 
        {
          outputImage(j, i, 0) = 1;
          outputImage(j, i, 1) = 0.5;
          outputImage(j, i, 2) = 0.5;
          outputImage(j, i, 3) = 0.5;
        }
        else 
        {
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
  
  return 0;
}
