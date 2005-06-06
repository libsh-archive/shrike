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

class Lafortune : public Shader {
public:
  Lafortune();
  ~Lafortune();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Lafortune instance;
};

Lafortune::Lafortune()
  : Shader("Basic Lighting Models: Lafortune")
{
}

Lafortune::~Lafortune()
{
}

bool Lafortune::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    ShOutputVector3f otan;
    ShOutputVector3f osurf;
    ShOutputVector3f lightv; // direction to light
    ShOutputVector3f viewv;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    otan = Globals::mv | itan; // Compute view-space tangent
    osurf = cross(onorm,otan);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
    ShPoint3f eye = -normalize(posv); // view vector
    viewv = normalize(eye - posv);

  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
	
  ShAttrib3f SH_DECL(D) = ShAttrib3f(0.0,0.0,1.0);
  D.range(-5.0,5.0);
	
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(1.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
    ShInputVector3f tangent;
    ShInputVector3f surface;
    ShInputVector3f light;
    ShInputVector3f view;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    tangent = normalize(tangent);
    surface = normalize(surface);
    light = normalize(light);

    ShVector3f lS = ShVector3f(light|tangent, light|surface, light|normal);
    lS *= D;
    ShVector3f vS = ShVector3f(view|tangent, view|surface, view|normal);
				
    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    result = diffuse*irrad + specular*pow(pos(lS|vS), exponent);
		
  } SH_END;
  return true;
}

Lafortune Lafortune::instance = Lafortune();

