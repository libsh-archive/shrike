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
#include "HDRTonemap.hpp"
#include "HDRMipmap.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class AshikhminToneMapShader : public Shader {
public:
  AshikhminToneMapShader();
  ~AshikhminToneMapShader();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
	
	std::string fname;

	static AshikhminToneMapShader instance;
};

AshikhminToneMapShader::AshikhminToneMapShader()
  : Shader("HDR: ToneMapping: Ashikhmin"), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

AshikhminToneMapShader::~AshikhminToneMapShader()
{}

bool AshikhminToneMapShader::init()
{
	HDRImage image;

	std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
	image.loadHDR(filename.c_str());

	AshikhminToneMap<ShUnclamped<ShTextureRect<ShVector4f> > > ToneMapImg(image.width(), image.height());
	ToneMapImg.internal(true);
	ToneMapImg.memory(image.memory());
  
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

	fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInputPosition4f posh;
	  ShInputTexCoord2f tc;
	  ShInputNormal3f normal;

		ShOutputColor3f result;
		
		// display the image
		ShAttrib3f RGB = pow(2, level + 2.47393) * ToneMapImg(tc)(0,1,2);
		
		ShAttrib1f f = 0.184874;
		ShAttrib1f e = 2.718281828;
		RGB(0) = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
		RGB(1) = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
		RGB(2) = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
		ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
		result = 0.285714286 * pow(RGB,gammainv(0,0,0));
	} SH_END;

  return true;
}

AshikhminToneMapShader AshikhminToneMapShader::instance = AshikhminToneMapShader();

class ReinhardToneMapShader : public Shader {
public:
  ReinhardToneMapShader();
  ~ReinhardToneMapShader();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
	
	std::string fname;

	static ReinhardToneMapShader instance;
};

ReinhardToneMapShader::ReinhardToneMapShader()
  : Shader("HDR: ToneMapping: Reinhard"), fname("memorial.hdr")
{
	setStringParam("Image Name", fname);
}

ReinhardToneMapShader::~ReinhardToneMapShader()
{}

bool ReinhardToneMapShader::init()
{
	HDRImage image;

	std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
	image.loadHDR(filename.c_str());
	
	ReinhardToneMap<ShUnclamped<ShTextureRect<ShVector4f> > > ToneMapImg(image.width(), image.height());
	ToneMapImg.internal(true);
	ToneMapImg.memory(image.memory());
	
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

	fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInputPosition4f posh;
	  ShInputTexCoord2f tc;
	  ShInputNormal3f normal;

		ShOutputColor3f result;
		
		// display the image
		ShAttrib3f RGB = pow(2, level + 2.47393) * ToneMapImg(tc)(0,1,2);
		
		ShAttrib1f f = 0.184874;
		ShAttrib1f e = 2.718281828;
		RGB(0) = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
		RGB(1) = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
		RGB(2) = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
		ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
		result = 0.285714286 * pow(RGB,gammainv(0,0,0));
	} SH_END;

  return true;
}

ReinhardToneMapShader ReinhardToneMapShader::instance = ReinhardToneMapShader();

