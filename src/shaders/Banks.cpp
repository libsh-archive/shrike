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

class Banks : public Shader {
public:
  Banks();
  ~Banks();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Banks instance;
};

Banks::Banks()
  : Shader("Basic Lighting Models: Banks")
{
}

Banks::~Banks()
{
}

bool Banks::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f tangent;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f tangentv; // tangent to the surface
    ShOutputVector3f lightv; // direction to light
    ShOutputVector3f eyev; // direction to the eye
    ShOutputVector3f halfv; // half vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
 
    tangentv = Globals::mv | tangent;
    
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
    //lightv = (mToTangent | lightv);

    ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    //eyev = (mToTangent | viewv); // Compute eye direction

    halfv = normalize(viewv + lightv); // Compute half vector
    
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);
  ShAttrib1f SH_DECL(compensation) = ShAttrib1f(1.2);
  compensation.range(1.0f, 10.0f); // to combat the "excess brightness"
  ShAttrib1f tangent_modif = ShAttrib1f(0.0);
  tangent_modif.name("tangent rotation");
  tangent_modif.range(0.0f, 1.0f); // to change the tangent   
   
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f tangent;
    ShInputVector3f light;
    ShInputVector3f eye;
    ShInputVector3f half;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    // to rotate the tangent, a orthogonal vector is computed, then added to the tangent
    tangent = (1.0-tangent_modif) * tangent + tangent_modif * cross(tangent, normal);
    tangent = normalize(tangent);
    light = normalize(light);
    eye = normalize(eye);
    half = normalize(half);

    ShAttrib1f irrad = pos(normal | light);
    ShAttrib1f lightDotTan = light | tangent;
    ShAttrib1f brightcomp = sqrt(1.0 - lightDotTan*lightDotTan);
    ShAttrib1f viewDotTan = eye | tangent;
    result = diffuse * irrad * pow(brightcomp, compensation) +
             specular * pow(pos(brightcomp * sqrt(1.0 - viewDotTan*viewDotTan) - lightDotTan*viewDotTan), exponent);
    result = result * (normal | light);
    
  } SH_END;
  return true;
}

Banks Banks::instance = Banks();


