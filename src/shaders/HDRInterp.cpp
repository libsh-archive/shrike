#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"
#include "HDRInterp.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"


class testInterpolation : public Shader {
public:
  testInterpolation(std::string type);
  ~testInterpolation();

  void createMaps(const ShImage&, ShImage&, ShImage&);
  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	virtual void initInterp() = 0;
	
  ShProgram vsh, fsh;
	ShProgram tonemapping;
	
	std::string fname;

};

testInterpolation::testInterpolation(std::string type)
  : Shader(std::string("HDR: Interpolation: ") + type), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

testInterpolation::~testInterpolation()
{}

bool testInterpolation::init()
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

	initInterp();
	
  return true;
}

class testLinearInterpolation: public testInterpolation {
public:
	testLinearInterpolation(): testInterpolation("Linear") {};
	
	static testLinearInterpolation instance;
	
	void initInterp() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
		Img.internal(true);
		Img.memory(image.memory());
	
		LinInterp<ShUnclamped<ShTextureRect<ShVector4f> > > LinImg(image.width(), image.height());
		LinImg.internal(true);
		LinImg.memory(image.memory());

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(interpolation) =  ShAttrib1f(0.0);
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			ShOutputAttrib4f interp = cond(interpolation > 0.5, Img(tc), LinImg(tc));
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testLinearInterpolation testLinearInterpolation::instance = testLinearInterpolation();

class testCubicInterpolation: public testInterpolation {
public:
	testCubicInterpolation(): testInterpolation("Catmull-Rom spline") {};
	
	static testCubicInterpolation instance;
	
	void initInterp() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
		Img.internal(true);
		Img.memory(image.memory());
	
		BicubicInterp<ShUnclamped<ShTextureRect<ShVector4f> > > BicubImg(image.width(), image.height());
		BicubImg.internal(true);
		BicubImg.memory(image.memory());

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(interpolation) =  ShAttrib1f(0.0);
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			ShOutputAttrib4f interp = cond(interpolation > 0.5, Img(tc), BicubImg(tc));
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testCubicInterpolation testCubicInterpolation::instance = testCubicInterpolation();

class testBSplineInterpolation: public testInterpolation {
public:
	testBSplineInterpolation(): testInterpolation("cubic B-spline") {};
	
	static testBSplineInterpolation instance;
	
	void initInterp() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
		Img.internal(true);
		Img.memory(image.memory());
	
		CubicBSplineInterp<ShUnclamped<ShTextureRect<ShVector4f> > > BSplineImg(image.width(), image.height());
		BSplineImg.internal(true);
		BSplineImg.memory(image.memory());

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(interpolation) =  ShAttrib1f(0.0);
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale * BSplineImg.size();
			ShOutputAttrib4f interp = cond(interpolation > 0.5, Img[tc], BSplineImg[tc]);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testBSplineInterpolation testBSplineInterpolation::instance = testBSplineInterpolation();

