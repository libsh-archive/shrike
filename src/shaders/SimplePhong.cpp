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
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class SimplePhong : public Shader {
public:
  SimplePhong();
  ~SimplePhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SimplePhong instance;
};

SimplePhong::SimplePhong()
  : Shader("Basic Lighting Models: Phong: Classic Phong")
{
}

SimplePhong::~SimplePhong()
{
}

bool SimplePhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f view;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f view;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    normal = normalize(normal);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    result = diffuse * irrad + specular * pow(pos(view | reflect(light, normal)), exponent);
  } SH_END;
  return true;
}

SimplePhong SimplePhong::instance = SimplePhong();


// the modified Phong model
class ModifiedPhong : public Shader {
public:
  ModifiedPhong();
  ~ModifiedPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static ModifiedPhong instance;
};

ModifiedPhong::ModifiedPhong()
  : Shader("Basic Lighting Models: Phong: Modified Phong")
{
}

ModifiedPhong::~ModifiedPhong()
{
}

bool ModifiedPhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f view;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f view;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    
    result = irrad * (diffuse/M_PI + specular*(exponent+2)/(2*M_PI) * pow(pos(view | reflect(light, normal)), exponent));
  } SH_END;
  return true;
}

ModifiedPhong ModifiedPhong::instance = ModifiedPhong();

// the Blinn-Phong model
class BlinnPhong : public Shader {
public:
  BlinnPhong();
  ~BlinnPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static BlinnPhong instance;
};

BlinnPhong::BlinnPhong()
  : Shader("Basic Lighting Models: Phong: Blinn-Phong")
{
}

BlinnPhong::~BlinnPhong()
{
}

bool BlinnPhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f halfv;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f half;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    result = diffuse * irrad + specular * pow(pos(normal | half), exponent) / (normal | light);
  } SH_END;
  return true;
}

BlinnPhong BlinnPhong::instance = BlinnPhong();


