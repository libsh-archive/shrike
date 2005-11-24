// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
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
  CookTorranceBeckmann(const Globals&);
  ~CookTorranceBeckmann();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

CookTorranceBeckmann::CookTorranceBeckmann(const Globals& globals)
  : Shader("Basic Lighting Models: Cook-Torrance: Beckmann's model", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction
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
    ShAttrib1f G = sat(SH::min(X * normalDotLight, X * normalDotEye));

    ShAttrib1f CT = (D*F*G) / (normalDotLight * normalDotEye * M_PI); // Compute Cook-Torrance lighting
    
    ShAttrib3f specular = color * max(0.0, CT);
    ShAttrib3f diffuse = color * max(0.0, normalDotLight/M_PI);
		result = diffuse + specular;
    
  } SH_END;
  return true;
}


class CookTorranceBlinn : public Shader {
public:
  CookTorranceBlinn(const Globals&);
  ~CookTorranceBlinn();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

CookTorranceBlinn::CookTorranceBlinn(const Globals& globals)
  : Shader("Basic Lighting Models: Cook-Torrance: Blinn's model", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction

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
    ShAttrib1f G = sat(SH::min(X * normalDotLight, X * normalDotEye));

    ShAttrib1f CT = (D*F*G) / (normalDotLight * normalDotEye * M_PI); // Compute Cook-Torrance lighting
    
    ShAttrib3f specular = color * max(0.0, CT);
    ShAttrib3f diffuse = color * max(0.0, normalDotLight/M_PI);
    result = diffuse + specular;
    
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new CookTorranceBeckmann(globals));
    list.push_back(new CookTorranceBlinn(globals));
    return list;
  }
}
#else
static StaticLinkedShader<CookTorranceBeckmann> instance = 
       StaticLinkedShader<CookTorranceBeckmann>();
static StaticLinkedShader<CookTorranceBlinn> instance2 = 
       StaticLinkedShader<CookTorranceBlinn>();
#endif
