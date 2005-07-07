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
#include <iostream>
#include <fstream>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include "ShrikeGl.hpp"
#include "Shader.hpp"
#include "Globals.hpp"
#include "Text.hpp"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sh/ShObjMesh.hpp>

using namespace SH;
using namespace ShUtil;

class Logo : public Shader {
public:
  Logo();
  ~Logo();

  bool init();

  void bind();
  
  void render();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh_h;}
  
  ShProgram vsh;
  ShProgram fsh_h, fsh_s;

private:

  ShPoint3f saw, sbw;

  ShObjMesh* m_model;

  ShProgramSet* m_shadow_set;
  ShProgramSet* m_object_set;
  
  static Logo* instance;
};

Logo::Logo()
  : Shader("Vector Graphics: Sh Logo"),
    m_shadow_set(0),
    m_object_set(0)
{
  saw = ShPoint3f(0, 1.35, 0);
  sbw = ShPoint3f(1, 1.35, 0);
  saw.internal(true);
  sbw.internal(true);
  SH_NAME(saw);
  SH_NAME(sbw);
}

Logo::~Logo()
{
  delete m_shadow_set;
  delete m_object_set;
}

void Logo::bind()
{
}

void Logo::render()
{
  shBind(*m_shadow_set);

  float r = 10.0;
  
  glBegin(GL_QUADS); {
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f( r, r);
    glVertex3f( r, 0.0,  r);
    glTexCoord2f(-r, r);
    glVertex3f(-r, 0.0,  r);
    glTexCoord2f(-r,-r);
    glVertex3f(-r, 0.0, -r);
    glTexCoord2f( r,-r);
    glVertex3f( r, 0.0, -r);
  } glEnd();

  shBind(*m_object_set);
  
  float values[4];
  glBegin(GL_TRIANGLES);
  for(ShObjMesh::FaceSet::iterator I = m_model->faces.begin();
      I != m_model->faces.end(); ++I) {
    ShObjEdge *e = (*I)->edge;
    do {
      e->normal.getValues(values); 
      glNormal3fv(values);

      e->texcoord.getValues(values);
#ifdef GL_ARB_multitexture
      glMultiTexCoord2fvARB(GL_TEXTURE0, values);
#else
      glMultiTexCoord2fv(GL_TEXTURE0, values);
#endif

      e->tangent.getValues(values);
#ifdef GL_ARB_multitexture
      glMultiTexCoord2fvARB(GL_TEXTURE0 + 1, values);
#else
      glMultiTexCoord2fv(GL_TEXTURE0 + 1, values);
#endif

      e->start->pos.getValues(values);
      glVertex3fv(values);
      e = e->next;
    } while(e != (*I)->edge);
  }
  glEnd();

}

bool Logo::init()
{
  //ShEnvironment::optimizationLevel = 0;
  std::cerr << "Initializing " << name() << std::endl;

  std::ifstream infile(SHMEDIA_DIR "/objs/s.obj");
  if (infile) {
    m_model = new ShObjMesh(infile);
  } else {
    return false;
  }
  
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "normal", "lightVec", "posh") << vsh;
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;

  ShProgram warper = SH_BEGIN_PROGRAM() {
    ShPoint3f SH_DECL(l);
    ShPoint2f SH_DECL(a);
    ShPoint2f SH_DECL(b);
    ShInputTexCoord2f SH_DECL(i);
    ShOutputTexCoord2f SH_DECL(o);

    ShPoint3f SH_DECL(lightPos) = Globals::lightDirW * Globals::lightLenW;
    
    a(0) = (lightPos(1)*saw(0) - lightPos(0)*saw(1)) /(lightPos(1) - saw(1));
    a(1) = (lightPos(1)*saw(2) - lightPos(2)*saw(1)) /(lightPos(1) - saw(1));
    b(0) = (lightPos(1)*sbw(0) - lightPos(0)*sbw(1)) /(lightPos(1) - sbw(1));
    b(1) = (lightPos(1)*sbw(2) - lightPos(2)*sbw(1)) /(lightPos(1) - sbw(1));

    ShAttrib1f dx = i(1)/a(1) * ((0.0 - a(0)) + (b(0) - 1.0)) + 1.0;

    o(0) = (i(0) - a(0)*i(1)/a(1))/dx;
    o(1) = i(1)/a(1);

    o = cond(lightPos(1) < saw(1), ShTexCoord2f(-1.0, -1.0), o);
  } SH_END;
  
  ShProgram scaler_s = SH_BEGIN_PROGRAM() {
    ShInOutTexCoord2f tc;
    tc *= ShAttrib2f(36.0, 60.0);
  } SH_END;

  ShProgram scaler_h = SH_BEGIN_PROGRAM() {
    ShInOutTexCoord2f tc;
    tc *= ShAttrib2f(45.0, 60.0);
  } SH_END;

  ShColor3f SH_DECL(backdrop) = ShColor3f(1.0, 1.0, 1.0);
  ShAttrib1f SH_DECL(mindiff) = 0.0;
  
  ShProgram h_renderer = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputAttrib1f in;
    ShInputNormal3f n;
    ShInputVector3f l;
    l = normalize(l);
    n = normalize(n);
    ShOutputColor3f out = cond(in,
                               ShColor3f(0.0, 0.0, 0.0),
                               backdrop * max(n|l, mindiff));
  } SH_END;

  ShColor3f SH_DECL(color) = ShColor3f(0.5, 0.5, 0.5);
  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  
  ShProgram s_renderer = lose<ShTexCoord2f>() & ShKernelSurface::diffuse<ShColor3f>() << color << lightColor;

  fsh_s = s_renderer;
  fsh_h = h_renderer << doText("h") << scaler_h << warper;

  delete m_shadow_set;
  delete m_object_set;

  m_object_set = new ShProgramSet(vsh, fsh_s);
  m_shadow_set = new ShProgramSet(vsh, fsh_h);
  
  return true;
}

Logo* Logo::instance = new Logo();
