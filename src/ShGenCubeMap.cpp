#include <iostream>
#include <fstream>
#include <string>
#include <sh/sh.hpp>
#include "HDRImage.hpp"

int main(int argc, char **argv) {
	if(argc < 2) {
		cout<<"format: shgencubemap [file]"<<endl;
		return -1;
	}
 	string inFileName(argv[1]);
 	string outFileName = inFileName.substr(0,inFileName.size() - 4) +  "_cubemap.hdr";
	HDRImage img;
	img.loadHDR(inFileName.c_str());
	img.CreateHDRCubeMap(outFileName.c_str());
	return 0;
}
