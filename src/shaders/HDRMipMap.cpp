#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"
#include "HDRMipmap.hpp"
#include "HDRInterp.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class testMipMap : public Shader {
public:
  testMipMap(std::string type);
  ~testMipMap();

  void createMaps(const ShImage&, ShImage&, ShImage&);
  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	virtual void initMipMap() = 0;
	
  ShProgram vsh, fsh;
	ShProgram tonemapping;
	
	std::string fname;

  static testMipMap instance;
};

testMipMap::testMipMap(std::string type)
  : Shader(std::string("HDR: Mip-Mapping: ") + type), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

testMipMap::~testMipMap()
{
}

bool testMipMap::init()
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
		
		result	= ShColor3f(R,G,B) * 0.285;
	} SH_END;

	initMipMap();
	
	return true;
}

class testSimpleMipMap: public testMipMap {
public:
	testSimpleMipMap(): testMipMap("Mip-Mapping") {};
	
	static testSimpleMipMap instance;
	
	void initMipMap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > MipMapImg(image.width(), image.height());
		MipMapImg.internal(true);
		MipMapImg.memory(image.memory());
		MipMapImg.updateMipMap();

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(mipmaplevel) = ShAttrib1f(0.0);
		mipmaplevel.range(0.0,8.0);
		mipmaplevel.name("mipmap level");
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			MipMapImg.setLevel(mipmaplevel);
			ShOutputAttrib4f result = MipMapImg(tc);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testSimpleMipMap testSimpleMipMap::instance = testSimpleMipMap();

class testLinMipMap: public testMipMap {
public:
	testLinMipMap(): testMipMap("Bilinear Interpolation") {};
	
	static testLinMipMap instance;
	
	void initMipMap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		LinInterp<MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > > MipMapImg(image.width(), image.height());
		MipMapImg.internal(true);
		MipMapImg.memory(image.memory());
		MipMapImg.updateMipMap();

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(mipmaplevel) = ShAttrib1f(0.0);
		mipmaplevel.range(0.0,8.0);
		mipmaplevel.name("mipmap level");
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			MipMapImg.setLevel(mipmaplevel);
			ShOutputAttrib4f result = MipMapImg(tc);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testLinMipMap testLinMipMap::instance = testLinMipMap();

class testCubicMipMap: public testMipMap {
public:
	testCubicMipMap(): testMipMap("Catmull-Rom Interpolation") {};
	
	static testCubicMipMap instance;
	
	void initMipMap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		BicubicInterp<MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > > MipMapImg(image.width(), image.height());
		MipMapImg.internal(true);
		MipMapImg.memory(image.memory());
		MipMapImg.updateMipMap();

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(mipmaplevel) = ShAttrib1f(0.0);
		mipmaplevel.range(0.0,8.0);
		mipmaplevel.name("mipmap level");
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			MipMapImg.setLevel(mipmaplevel);
			ShOutputAttrib4f result = MipMapImg(tc);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testCubicMipMap testCubicMipMap::instance = testCubicMipMap();

class testBSplineMipMap: public testMipMap {
public:
	testBSplineMipMap(): testMipMap("B-Spline Interpolation") {};
	
	static testBSplineMipMap instance;
	
	void initMipMap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		CubicBSplineInterp<MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > > MipMapImg(image.width(), image.height());
		MipMapImg.internal(true);
		MipMapImg.memory(image.memory());
		MipMapImg.updateMipMap();

		ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
		scale.range(0.1,10.0);

		ShAttrib1f SH_DECL(mipmaplevel) = ShAttrib1f(0.0);
		mipmaplevel.range(0.0,8.0);
		mipmaplevel.name("mipmap level");
	
	  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
	    ShInputPosition4f posh;
	    ShInputTexCoord2f tc;
	    ShInputNormal3f normal;
	 		tc *= scale;
			MipMapImg.setLevel(mipmaplevel);
			ShOutputAttrib4f result = MipMapImg(tc);
		} SH_END;
	
		fsh = tonemapping << fsh;
	}
};

testBSplineMipMap testBSplineMipMap::instance = testBSplineMipMap();

