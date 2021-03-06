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

class SimplePhong : public Shader {
public:
  SimplePhong(const Globals &);
  ~SimplePhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

SimplePhong::SimplePhong(const Globals &globals)
  : Shader("Basic Lighting Models: Phong: Classic Phong", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction

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



// the modified Phong model
class ModifiedPhong : public Shader {
public:
  ModifiedPhong(const Globals &);
  ~ModifiedPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

ModifiedPhong::ModifiedPhong(const Globals &globals)
  : Shader("Basic Lighting Models: Phong: Modified Phong", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction

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


// the Blinn-Phong model
class BlinnPhong : public Shader {
public:
  BlinnPhong(const Globals &);
  ~BlinnPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

BlinnPhong::BlinnPhong(const Globals &globals)
  : Shader("Basic Lighting Models: Phong: Blinn-Phong", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction

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

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new SimplePhong(globals));
    list.push_back(new ModifiedPhong(globals));
    list.push_back(new BlinnPhong(globals));
    return list;
  }
}
#else
static StaticLinkedShader<SimplePhong> sp_instance = 
       StaticLinkedShader<SimplePhong>();
static StaticLinkedShader<ModifiedPhong> mp_instance = 
       StaticLinkedShader<ModifiedPhong>();
static StaticLinkedShader<BlinnPhong> bp_instance = 
       StaticLinkedShader<BlinnPhong>();
#endif
