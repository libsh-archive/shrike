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
#include "HDRCubeMap.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class testShinyBumpMapShader : public Shader {
public:
  ShProgram vsh, fsh;

  testShinyBumpMapShader();
  ~testShinyBumpMapShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  std::string fname;
	
};

testShinyBumpMapShader::testShinyBumpMapShader()
  : Shader("HDR: Shiny Bump Mapping"), fname("Cf_cubemap.hdr")
{
  setStringParam("Image Name", fname);
}

testShinyBumpMapShader::~testShinyBumpMapShader()
{
}

bool testShinyBumpMapShader::init()
{
  HDRImage image;
  std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
  image.loadHDR(filename.c_str());
  CubeMap<ShUnclamped<ShTextureRect<ShVector4f> > > Img(image.width(), image.height());
  Img.internal(true);
  Img.memory(image.memory());

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc;    // texture coordinates
    ShOutputNormal3f onorm; // view-space normal
    ShOutputVector3f otan; // view-space tangent
    ShOutputVector3f viewv;  // view vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    otan = Globals::mv | itan; // Compute view-space tangent
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    viewv = -ShVector3f(posv); // Compute view vector
  } SH_END;

  ShImage bumpimage;
  bumpimage.loadPng(SHMEDIA_DIR "/bumpmaps/bumps_normals.png");
  ShTexture2D<ShVector3f> bump(bumpimage.width(),bumpimage.height());
  bump.memory(bumpimage.memory());

  ShAttrib3f SH_DECL(scale) = ShAttrib3f(2.0,2.0,1.0);
  scale.range(0.0f,10.0f);
  
  ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
  level.range(-10.0,10.0);  

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f n;
    ShInputVector3f t;
    ShInputVector3f v;

    ShOutputColor3f result;
    
    ShVector3f s = cross(t,n);
    t = normalize(t);
    s = normalize(s);
    n = normalize(n);
    ShVector3f b = bump(u) - ShAttrib3f(0.5,0.5,0.0);
    b *= scale;
    ShVector3f bn = t * b(0) + s * b(1) + n * b(2);
    ShVector3f r = reflect(v,bn); // Compute reflection vector

    // actually do reflection lookup in model space
    r = Globals::mv_inverse | r;

    result = Img(r)(0,1,2); 

    // display the image
    ShAttrib1f nlevel = pow (2, level + 2.47393);
    ShAttrib3f RGB = nlevel * result(0,1,2);
    
    ShAttrib1f f = 0.184874;
    ShAttrib1f e = 2.718281828;
    ShAttrib1f R = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
    ShAttrib1f G = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
    ShAttrib1f B = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
    ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
    R = pow(R,gammainv);
    G = pow(G,gammainv);
    B = pow(B,gammainv);
    
    result = ShColor3f(R,G,B) * 0.285714286; // scale to get the correct range	(0.285714286=1/3.5)
  } SH_END;

  return true;
}


testShinyBumpMapShader the_hdr_shinybumpmap_shader;
