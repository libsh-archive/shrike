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
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"

using namespace SH;
using namespace ShUtil;

class SpatialBRDF : public Shader {
public:
  SpatialBRDF();
  ~SpatialBRDF();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SpatialBRDF instance;

  std::string fname;
	
};

SpatialBRDF::SpatialBRDF()
  : Shader("HDR: Spatial BRDF"), fname("Af_cubemap")
{
  setStringParam("Image Name", fname);
}

SpatialBRDF::~SpatialBRDF()
{
}

bool SpatialBRDF::init()
{
  HDRImage diffuseimg, specularimg;
  std::string diffusename = SHMEDIA_DIR "/hdr/hdr/" + fname + "_SBRDF_diffuse.hdr";
  diffuseimg.loadHDR(diffusename.c_str());
  ShUnclamped<ShTextureRect<ShVector4f> > DiffImg(diffuseimg.width(), diffuseimg.height());
  DiffImg.memory(diffuseimg.memory());
  DiffImg.internal(true);
	
  std::string specularname = SHMEDIA_DIR "/hdr/hdr/" + fname + "_SBRDF_specular_128.hdr";
  specularimg.loadHDR(specularname.c_str());
  ShUnclamped<ShTextureRect<ShVector4f> > SpecImg(specularimg.width(), specularimg.height());
  SpecImg.memory(specularimg.memory());
  SpecImg.internal(true);
	
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    ShOutputVector3f view;
    ShOutputVector3f reflv;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);
    reflv = reflect(viewv,onorm); // Compute reflection vector
    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;	
 
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.6, 0.6, 0.6);
  ShColor3f SH_DECL(diffuse) = ShColor3f(0.6, 0.6, 0.6);
	
  ShAttrib3f SH_DECL(C) = ShAttrib3f(-1.0,-1.0,1.0);
  C.range(-1.0,1.0);

  ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
  level.range(-10.0,10.0);
	
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputNormal3f normal;
    ShInputVector3f view;
    ShInputVector3f refl;
		
    ShOutputColor3f result;

    normal = normalize(normal);
    view = normalize(view);
    refl = normalize(refl);
	
    ShAttrib3f p = C * view;
    ShAttrib1f normp = sqrt(p | p);
		
    ShAttrib3f absn = abs(refl);
    ShAttrib1f maxcoord = SH::max(SH::max(absn(0), absn(1)), absn(2));
    ShAttrib3f textcoord = -refl / maxcoord;
    ShAttrib1f Xface = textcoord(0) * (absn(0) > SH::max(absn(1), absn(2)));
    ShAttrib1f Yface = textcoord(1) * (absn(1) > SH::max(absn(0), absn(2)));
    ShAttrib1f Zface = textcoord(2) * (absn(2) > SH::max(absn(0), absn(1)));
    ShAttrib2f u = cond(abs(Xface), ShAttrib2f(textcoord(2),textcoord(1)), cond(abs(Yface), ShAttrib2f(textcoord(0), textcoord(2)), ShAttrib2f(textcoord(0), textcoord(1))));
    u += ShAttrib2f(1.0,1.0);
    u *= ShAttrib2f(0.25,0.1666666);
    u(0) = cond(Xface+Yface+Zface, u(0), u(0)+0.5);
    u(1) = cond(abs(Xface), u(1)+0.333333, cond(abs(Yface), u(1)+0.666667, u(1)));
		
    result = diffuse*DiffImg(u)(0,1,2) + specular * SpecImg(u)(0,1,2) * pow(normp,128) *  pos(normal | p);

    ShAttrib3f RGB = pow(2, level + 2.47393) * result;
		
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
  return true;
}

SpatialBRDF SpatialBRDF::instance = SpatialBRDF();
