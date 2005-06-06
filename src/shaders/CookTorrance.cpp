// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
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

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class CookTorranceBeckmann : public Shader {
public:
  CookTorranceBeckmann();
  ~CookTorranceBeckmann();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static CookTorranceBeckmann instance;
};

CookTorranceBeckmann::CookTorranceBeckmann()
  : Shader("Basic Lighting Models: Cook-Torrance: Beckmann's model")
{
}

CookTorranceBeckmann::~CookTorranceBeckmann()
{
}

bool CookTorranceBeckmann::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light
    ShOutputVector3f eyev; // direction to the eye
    ShOutputVector3f halfv; // half vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
    //lightv = (mToTangent | lightv);

    ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    //eyev = (mToTangent | viewv); // Compute eye direction

    halfv = normalize(viewv + lightv); // Compute half vector
    
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(0.2, 0.5, 0.9);
  ShAttrib1f SH_DECL(roughness) = ShAttrib1f(0.15);
  roughness.range(0.1f, 1.0f);
  ShAttrib1f SH_DECL(eta) = ShAttrib1f(1.2);
  eta.range(1.0f, 5.0f); // the relative index of refraction
   
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputVector3f eye;
    ShInputVector3f half;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);
    eye = normalize(eye);
    half = normalize(half);

    // Beckman's distribution function
    ShAttrib1f D = beckmann(normal, half, roughness);

    // Fresnel term
    ShAttrib1f F = fresnel(eye,normal,eta);

    // self shadowing term
    ShAttrib1f normalDotEye = pos(normal | eye);
    ShAttrib1f normalDotLight = pos(normal | light);
    ShAttrib1f X = 2.0 * pos(normal | half) / pos(eye | half);
    ShAttrib1f G = sat(min(X * normalDotLight, X * normalDotEye));

    ShAttrib1f CT = (D*F*G) / (normalDotLight * normalDotEye * M_PI); // Compute Cook-Torrance lighting
    
    ShAttrib3f specular = color * max(0.0, CT);
    ShAttrib3f diffuse = color * max(0.0, normalDotLight/M_PI);
		result = diffuse + specular;
    
  } SH_END;
  return true;
}

CookTorranceBeckmann CookTorranceBeckmann::instance = CookTorranceBeckmann();

class CookTorranceBlinn : public Shader {
public:
  CookTorranceBlinn();
  ~CookTorranceBlinn();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static CookTorranceBlinn instance;
};

CookTorranceBlinn::CookTorranceBlinn()
  : Shader("Basic Lighting Models: Cook-Torrance: Blinn's model")
{
}

CookTorranceBlinn::~CookTorranceBlinn()
{
}

bool CookTorranceBlinn::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light
    ShOutputVector3f eyev; // direction to the eye
    ShOutputVector3f halfv; // half vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    halfv = normalize(viewv + lightv); // Compute half vector
    
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(0.2, 0.5, 0.9);
  ShAttrib1f SH_DECL(roughness) = ShAttrib1f(0.15);
  roughness.range(0.1f, 1.0f);
  ShAttrib1f SH_DECL(eta) = ShAttrib1f(1.2);
  eta.range(1.0f, 5.0f); // the relative index of refraction
  ShAttrib1f constant = ShAttrib1f(10.0); // the arbitrary constant c
  constant.name("normalization constant");
  constant.range(0.0f, 50.0f);
   
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputVector3f eye;
    ShInputVector3f half;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);
    eye = normalize(eye);
    half = normalize(half);

    // Blinn distribution
    ShAttrib1f normalDotHalf = (normal | half);
    ShAttrib1f normalDotHalf2 = normalDotHalf * normalDotHalf;
    ShAttrib1f roughness2 = roughness * roughness; // roughness fixed at 0.15
    ShAttrib1f exponent = (normalDotHalf2 - 1) / roughness2; // Compute the exponent value
    ShAttrib1f D = constant * pow(M_E, exponent); // Compute the distribution function

    // Fresnel term
    ShAttrib1f F = fresnel(eye,normal,eta);

    // self shadowing term
    ShAttrib1f normalDotEye = pos(normal | eye);
    ShAttrib1f normalDotLight = pos(normal | light);
    ShAttrib1f X = 2.0 * normalDotHalf / pos(eye | half);
    ShAttrib1f G = sat(min(X * normalDotLight, X * normalDotEye));

    ShAttrib1f CT = (D*F*G) / (normalDotLight * normalDotEye * M_PI); // Compute Cook-Torrance lighting
    
    ShAttrib3f specular = color * max(0.0, CT);
    ShAttrib3f diffuse = color * max(0.0, normalDotLight/M_PI);
    result = diffuse + specular;
    
  } SH_END;
  return true;
}

CookTorranceBlinn CookTorranceBlinn::instance = CookTorranceBlinn();


