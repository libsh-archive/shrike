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

class Mipmap : public Shader {
public:
  Mipmap(std::string type);
  ~Mipmap();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	virtual void initMipMap() = 0;
	
  ShProgram vsh, fsh;
	ShProgram tonemapping;
	
	std::string fname;

  static Mipmap instance;
};

Mipmap::Mipmap(std::string type)
  : Shader(std::string("HDR: Mip-Mapping: ") + type), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

Mipmap::~Mipmap()
{
}

bool Mipmap::init()
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

class SimpleMipMap: public Mipmap {
public:
	SimpleMipMap(): Mipmap("Mip-Mapping") {};
	
	static SimpleMipMap instance;
	
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

SimpleMipMap SimpleMipMap::instance = SimpleMipMap();

class LinMipMap: public Mipmap {
public:
	LinMipMap(): Mipmap("Bilinear Interpolation") {};
	
	static LinMipMap instance;
	
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

LinMipMap LinMipMap::instance = LinMipMap();

class CubicMipMap: public Mipmap {
public:
	CubicMipMap(): Mipmap("Catmull-Rom Interpolation") {};
	
	static CubicMipMap instance;
	
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

CubicMipMap CubicMipMap::instance = CubicMipMap();

class BSplineMipMap: public Mipmap {
public:
	BSplineMipMap(): Mipmap("B-Spline Interpolation") {};
	
	static BSplineMipMap instance;
	
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

BSplineMipMap BSplineMipMap::instance = BSplineMipMap();

class CardSplineMipMap: public Mipmap {
public:
	CardSplineMipMap(): Mipmap("cardinal Spline Interpolation") {};
	
	static CardSplineMipMap instance;
	
	void initMipMap() {
		HDRImage image;

		std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
		image.loadHDR(filename.c_str());

		CardinalSplineInterp<MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > > MipMapImg(image.width(), image.height());
		MipMapImg.internal(true);
		MipMapImg.memory(image.memory());
		MipMapImg.updateCardSpline();
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

CardSplineMipMap CardSplineMipMap::instance = CardSplineMipMap();



