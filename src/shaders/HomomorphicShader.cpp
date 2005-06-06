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
#include "HDRInterp.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class HomomorphicShader : public Shader {
public:
  HomomorphicShader();
  ~HomomorphicShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static HomomorphicShader instance;
};

HomomorphicShader::HomomorphicShader()
  : Shader("Homomorphic Factorization: Garnet Red: Simple")
{
}

HomomorphicShader::~HomomorphicShader()
{
}

bool HomomorphicShader::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputTexCoord3f tc; // ignored for now, though
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputVector3f light; // direction to light (surface frame)
    ShOutputVector3f view;

    opos = Globals::mvp | ipos; // Compute NDC position
    ShNormal3f n = Globals::mv | inorm; // Compute view-space normal
    ShNormal3f t = Globals::mv | itan; // Compute view-space tangent
    n = normalize(n);
    t = normalize(t);

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f lightv = normalize(Globals::lightPos - posv); // Compute light direction
    ShVector3f viewv = -normalize(posv); // Compute view vector

    // compute local surface frame (in view space)
    // ShVector3f t = normalize(itan - (itan|n)*n);
    ShVector3f s = normalize(cross(t,n));
    
    // project view and light vectors onto local surface frame
    view(0) = (t|viewv);
    view(1) = (s|viewv);
    view(2) = (n|viewv);

    light(0) = (t|lightv);
    light(1) = (s|lightv);
    light(2) = (n|lightv);  // if positive, is irradiance scale
  } SH_END;

  ShImage image;

  // TODO: should have array of available BRDFs with correction
  // factor for each, hidden uniforms (don't want user to play with
  // alpha, really), pulldown menu to select BRDFs from list,
  // settings for extra specularities, etc. etc.
  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_0.png");
  ShTexture2D<ShColor3f> ptex(image.width(), image.height());
  //CubicBSplineInterp<ShTexture2D<ShColor3f> > ptex(image.width(), image.height());
  ptex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_1.png");
  ShTexture2D<ShColor3f> qtex(image.width(), image.height());
  //CubicBSplineInterp<ShTexture2D<ShColor3f> > qtex(image.width(), image.height());
  qtex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/specular.png");
  ShTexture2D<ShColor3f> stex(image.width(), image.height());
  stex.memory(image.memory());

  // these scale factors are specific to garnet red
  ShColor3f SH_DECL(alpha) = ShColor3f(0.0410592,0.0992037,0.0787714);
  ShAttrib1f SH_DECL(diffuse) = ShAttrib1f(5.0);
  diffuse.range(0.0,20.0);
  ShAttrib1f SH_DECL(specular) = ShAttrib1f(0.5);
  specular.range(0.0,1.0);
  ShColor3f SH_DECL(light_color) = ShColor3f(1.0,1.0,1.0);
  light_color.range(0.0,1.0);
  ShAttrib1f SH_DECL(light_power) = ShAttrib1f(1.0);
  light_power.range(0.0,100.0);
  
  ShAttrib1f SH_DECL(interpolation) = ShAttrib1f(0.0);
	  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputVector3f light;
    ShInputVector3f view;

    ShOutputColor3f result;

    // Normalize (theoretically not needed if compiler smart enough)
    light = normalize(light);
    view = normalize(view);

    // Incorporate diffuse scale, correction factor, and irradiance
    result = diffuse * alpha * pos(light(2));

    // compute half vector
    ShVector3f half = normalize(view + light); 

    // Theoretically not needed if use common subexpression elimination...
    ShTexCoord2f hu = parabolic_norm(half);
    ShTexCoord2f lu = parabolic_norm(light);
    ShTexCoord2f vu = parabolic_norm(view);

    // TODO: SHOULD use automatic projective normalization in texture lookup...
    // and/or parabolic texture type instead.
    result *= ptex(lu);
    result *= qtex(hu);
    result *= ptex(vu);

    // Add in specular term (also represented using parabolic map)
    result += specular * stex(hu) * pos(light(2));

    // Take into account light power and colour
    result *= light_power * light_color;
  } SH_END;
  return true;
}

// TODO: above code actually results in redundant normalization calls
// to light (it is normalized in both irradiance and in parabolic_norm).
// Rather than "fix" this code, this shader should be used as a test
// case (and an example) for automatic common subexpression elimination.
// Note that repeated normalization can be avoided with unit flags,
// so inserting the commented-out code block for normalization fixes
// the problem from an efficiency standpoint... even though vectors
// are normalized more than once in the code, the final assembly should
// only do it once.

HomomorphicShader HomomorphicShader::instance = HomomorphicShader();


