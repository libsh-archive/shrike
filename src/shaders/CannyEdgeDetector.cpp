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
#include "Filters.hpp"

using namespace SH;
using namespace ShUtil;
using namespace std;

#include "util.hpp"

#define TEST_BLURR 0

class CannyEdgeDetector : public Shader {
public:
  CannyEdgeDetector();
  ~CannyEdgeDetector();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

	std::string fname;

  static CannyEdgeDetector instance;
};

CannyEdgeDetector::CannyEdgeDetector()
  : Shader("Edge Detection: Canny"), fname("canaletto.png")
{
	setStringParam("Image Name", fname);
}

CannyEdgeDetector::~CannyEdgeDetector()
{
}

bool CannyEdgeDetector::init()
{
	ShImage image;
  std::string fileName(SHMEDIA_DIR "/edgedetection/" + fname);
	image.loadPng(fileName.c_str());
	GaussFilter<ShTextureRect<ShVector4f> > GaussImg(image.width(), image.height(), 1);
	AnisDiff<ShTextureRect<ShVector4f> > AnisDiffImg(image.width(), image.height());
	GaussImg.internal(true);
	GaussImg.memory(image.memory());
	AnisDiffImg.internal(true);
	AnisDiffImg.memory(image.memory());

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
		
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
  } SH_END;

	ShAttrib1f SH_DECL(hysteresis1) = ShAttrib1f(0.2);
  ShAttrib1f SH_DECL(blurType) = ShAttrib1f(0.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
 
		ShOutputColor3f result;
    
    tc *= GaussImg.size();
    ShAttrib4f lumvect(0.27,0.67,0.06,0.0); // to compute the luminance
#if (TEST_BLURR)
    ShAttrib1f Gx = lumvect | (GaussImg[tc] - GaussImg[tc+ShAttrib2f(1.0,1.0)]);
    ShAttrib1f Gy = lumvect | (GaussImg[tc+ShAttrib2f(1.0,0.0)] - GaussImg[tc+ShAttrib2f(0.0,1.0)]);
    Gx = cond(blurType > 0.5, Gx, lumvect | (AnisDiffImg[tc] - AnisDiffImg[tc+ShAttrib2f(1.0,1.0)]));
    Gy = cond(blurType > 0.5, Gy, lumvect | (AnisDiffImg[tc+ShAttrib2f(1.0,0.0)] - AnisDiffImg[tc+ShAttrib2f(0.0,1.0)]));
#else
    ShAttrib1f Gx = lumvect | (AnisDiffImg[tc] - AnisDiffImg[tc+ShAttrib2f(1.0,1.0)]);
    ShAttrib1f Gy = lumvect | (AnisDiffImg[tc+ShAttrib2f(1.0,0.0)] - AnisDiffImg[tc+ShAttrib2f(0.0,1.0)]);
#endif
    ShAttrib1f G = sqrt(Gx*Gx + Gy*Gy);
    G = cond(G > hysteresis1, G, ShAttrib1f(0.0));
    result = G(0,0,0);
    
	} SH_END;
  return true;
}

CannyEdgeDetector CannyEdgeDetector::instance = CannyEdgeDetector();
