#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"
#include "HDRMipmap.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"


class testWrap : public Shader {
public:
  testWrap(std::string type);
  ~testWrap();

  void createMaps(const ShImage&, ShImage&, ShImage&);
  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	virtual void initWrap() = 0;
	
  ShProgram vsh, fsh;
	ShProgram tonemapping;
	
	std::string fname;

};

testWrap::testWrap(std::string type)
  : Shader(std::string("HDR: Wrapping: ") + type), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

testWrap::~testWrap()
{}

bool testWrap::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
		
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

		ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
  } SH_END;

	ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
	level.range(-10.0,10.0);

	tonemapping = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInputAttrib4f interp;
		ShOutputColor3f result;
				
		// display the image
		level = pow (2, level + 2.47393);
		ShAttrib3f RGB = level * interp(0,1,2);
		
		ShAttrib1f f = 0.184874;
		ShAttrib1f e = 2.718281828;
		ShAttrib1f R = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
		ShAttrib1f G = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
		ShAttrib1f B = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
		ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
		R = pow(R,gammainv);
		G = pow(G,gammainv);
		B = pow(B,gammainv);
		
		result	= ShColor3f(R,G,B) * 0.285714286; // scale to get the correct range	(0.285714286=1/3.5)
	} SH_END;

	initWrap();
	
  return true;
}

class testWrapRepeat: public testWrap {
public:
	testWrapRepeat(): testWrap("Repeat") {};
	
	static testWrapRepeat instance;
	
	void initWrap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		MipMap<ShWrapRepeat<ShUnclamped<ShTextureRect<ShVector4f> > > > Img(image.width(), image.height());
		Img.internal(true);
		Img.memory(image.memory());
		Img.updateMipMap();
	
		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
			ShOutputAttrib4f interp = Img(scale*tc);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testWrapRepeat testWrapRepeat::instance = testWrapRepeat();

