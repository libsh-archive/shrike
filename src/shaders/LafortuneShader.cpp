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
#include <shutil/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Lafortune : public Shader {
public:
  Lafortune(const Globals&);
  ~Lafortune();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

Lafortune::Lafortune(const Globals& globals)
  : Shader("Basic Lighting Models: Lafortune", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal
    otan = m_globals.mv | itan; // Compute view-space tangent
    osurf = cross(onorm,otan);
    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction
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

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new Lafortune(globals));
    return list;
  }
}
#else
static StaticLinkedShader<Lafortune> instance = 
       StaticLinkedShader<Lafortune>();
#endif

