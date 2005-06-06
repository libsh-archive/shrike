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
#include "HDRInterp.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

/** the main class for the interpolatio examples
  * all examples are derivated from this class
  */
class Interpolation : public Shader {
public:
  Interpolation(std::string type);
  ~Interpolation();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  virtual void initInterp() = 0; // the fragment shader of the specific interpolations
  
  ShProgram vsh, fsh;
  ShProgram tonemapping;
  
  std::string fname;

};

Interpolation::Interpolation(std::string type)
  : Shader(std::string("HDR: Interpolation: ") + type), fname("memorial.hdr")
{
  setStringParam("Image Name", fname);
}

Interpolation::~Interpolation()
{}

bool Interpolation::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
  } SH_END;

  ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
  level.range(-10.0,10.0);

  tonemapping = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputAttrib4f interp;
    ShOutputColor3f result;
    
    // display the image
    ShAttrib3f RGB = pow(2, level + 2.47393) * interp(0,1,2);
    ShAttrib1f f = 0.184874;
    ShAttrib1f e = 2.718281828;
    ShAttrib1f R = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
    ShAttrib1f G = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
    ShAttrib1f B = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
    ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
    R = pow(R,gammainv);
    G = pow(G,gammainv);
    B = pow(B,gammainv);
    result  = ShColor3f(R,G,B) * 0.285714286; // scale to get the correct range (0.285714286=1/3.5)
  } SH_END;

  initInterp();
  
  return true;
}

/** an example of the bilinear interpolation
  */
class LinearInterpolation: public Interpolation {
public:
  LinearInterpolation(): Interpolation("Linear") {};
  
  static LinearInterpolation instance;
  
  void initInterp() 
  {
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

    ShAttrib1f SH_DECL(interpolation) = ShAttrib1f(0.0);

    fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
      ShInputPosition4f posh;
      ShInputTexCoord2f tc;
      ShInputNormal3f normal;
      tc *= scale;
      //ShOutputAttrib4f result = cond(interpolation > 0.5, Img(tc), LinImg(tc));
      ShOutputAttrib4f result = LinImg(tc);
    } SH_END;
  
    fsh = tonemapping << fsh;
  }
};

LinearInterpolation LinearInterpolation::instance = LinearInterpolation();

/** an example of the Catmull-Rom interpolation
  */
class CatmullRomInterpolation: public Interpolation {
public:
  CatmullRomInterpolation(): Interpolation("Catmull-Rom spline") {};
  
  static CatmullRomInterpolation instance;
  
  void initInterp()
  {
    HDRImage image;

    std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
    image.loadHDR(filename.c_str());

    ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
    Img.internal(true);
    Img.memory(image.memory());
  
    CatmullRomInterp<ShUnclamped<ShTextureRect<ShVector4f> > > CatmullRomImg(image.width(), image.height());
    CatmullRomImg.internal(true);
    CatmullRomImg.memory(image.memory());

    ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
    scale.range(0.1,10.0);

    ShAttrib1f SH_DECL(interpolation) = ShAttrib1f(0.0);
    
    fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
      ShInputPosition4f posh;
      ShInputTexCoord2f tc;
      ShInputNormal3f normal;
      tc *= scale;
      ShOutputAttrib4f interp = cond(interpolation > 0.5, Img(tc), CatmullRomImg(tc));
    } SH_END;
  
    fsh = tonemapping << fsh;
  }
};

CatmullRomInterpolation CatmullRomInterpolation::instance = CatmullRomInterpolation();


/**  an example of the B-Spline interpolation
  */
class BSplineInterpolation: public Interpolation {
public:
  BSplineInterpolation(): Interpolation("cubic B-spline") {};
  
  static BSplineInterpolation instance;
  
  void initInterp()
  {
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
      tc *= scale;
      ShOutputAttrib4f interp = cond(interpolation > 0.5, Img(tc), BSplineImg(tc));
    } SH_END;
  
    fsh = tonemapping << fsh;
  }
};

BSplineInterpolation BSplineInterpolation::instance = BSplineInterpolation();

/** an example of interpolation with cardinal splines
  */
class CardSplineInterpolation: public Interpolation {
public:
  CardSplineInterpolation(): Interpolation("cardinal Spline") {};
  
  static CardSplineInterpolation instance;
  
  void initInterp()
  {
    HDRImage image;

    std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
    image.loadHDR(filename.c_str());

    ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
    Img.internal(true);
    Img.memory(image.memory());
  
    CardinalSplineInterp<ShUnclamped<ShTextureRect<ShVector4f> > > CardSplineImg(image.width(), image.height());
    CardSplineImg.internal(true);
    CardSplineImg.memory(image.memory());

    ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
    scale.range(0.1,10.0);

    ShAttrib1f SH_DECL(interpolation) =  ShAttrib1f(0.0);
  
    fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
      ShInputPosition4f posh;
      ShInputTexCoord2f tc;
      ShInputNormal3f normal;
      tc *= scale;
      ShOutputAttrib4f interp = cond(interpolation > 0.5, Img(tc), CardSplineImg(tc));
    } SH_END;
  
    fsh = tonemapping << fsh;
  }
};

CardSplineInterpolation CardSplineInterpolation::instance = CardSplineInterpolation();


/** an example of interpolation using smooth splines
  * not derivated like the other classes because of the use of an update() function
  */
class SmoothSplineInterpolation : public Shader {
public:
  SmoothSplineInterpolation();
  ~SmoothSplineInterpolation();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
  
  std::string lambda;

  static SmoothSplineInterpolation instance;
};

SmoothSplineInterpolation::SmoothSplineInterpolation()
  : Shader(std::string("HDR: Interpolation: smooth Spline")), lambda("5")
{
  setStringParam("coefficient", lambda);
}

SmoothSplineInterpolation::~SmoothSplineInterpolation()
{}

bool SmoothSplineInterpolation::init()
{
  HDRImage image;
  std::string filename = SHMEDIA_DIR "/hdr/hdr/memorial.hdr";
  image.loadHDR(filename.c_str());

  ShUnclamped<ShTextureRect<ShVector4f> > Img(image.width(), image.height());
  Img.internal(true);
  Img.memory(image.memory());

  SmoothSplineInterp<ShUnclamped<ShTextureRect<ShVector4f> > > SmoothSplineImg(image.width(), image.height());
  SmoothSplineImg.internal(true);
  SmoothSplineImg.memory(image.memory());
  SmoothSplineImg.setCoeff(atol(lambda.c_str()));

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

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
  scale.range(0.1,10.0);

  ShAttrib1f SH_DECL(interpolation) =  ShAttrib1f(0.0);
    
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
    
    ShOutputColor3f result;
    
    tc *= scale;
    result = cond(interpolation > 0.5, Img(tc)(0,1,2), SmoothSplineImg(tc)(0,1,2));
    
    // display the image
    ShAttrib3f RGB = pow(2, level + 2.47393) * result(0,1,2);
    ShAttrib1f f = 0.184874;
    ShAttrib1f e = 2.718281828;
    ShAttrib1f R = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
    ShAttrib1f G = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
    ShAttrib1f B = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
    ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
    R = pow(R,gammainv);
    G = pow(G,gammainv);
    B = pow(B,gammainv);
    result  = ShColor3f(R,G,B) * 0.285714286; // scale to get the correct range (0.285714286=1/3.5)
  } SH_END;

  return true;
}

SmoothSplineInterpolation SmoothSplineInterpolation::instance = SmoothSplineInterpolation();
