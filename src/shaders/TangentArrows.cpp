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

#include <wx/wx.h>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include "ShrikeGl.hpp"
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"
#include "ShrikeCanvas.hpp"

using namespace SH;
using namespace ShUtil;

class TangentArrows : public Shader {
public:
  TangentArrows()
    : Shader("Debugging: Tangent Arrows")
  {}
  
  ~TangentArrows() {}

  bool init();

  void render();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static TangentArrows instance;
};

void TangentArrows::render()
{

  glPushAttrib(GL_LINE_BIT);

  glLineWidth(2.0);
  
  float vs[4];
  glBegin(GL_LINES);
  const ShUtil::ShObjMesh& m_obj = *ShrikeCanvas::instance()->getModel();
  for(ShObjMesh::FaceSet::const_iterator I = m_obj.faces.begin();
      I != m_obj.faces.end(); ++I) {
    ShObjMesh::Edge* e = (*I)->edge;
    do {
      
      for (int i = 0; i < 2; i++) {
        e->normal.getValues(vs); glNormal3fv(vs);

        float f = (i ? 1.0 : 0.0);
#ifdef GL_ARB_multitexture
        glMultiTexCoord1fvARB(GL_TEXTURE0, &f);        
#else
        glMultiTexCoord1fv(GL_TEXTURE0, &f);
#endif
      
        e->tangent.getValues(vs);
#ifdef GL_ARB_multitexture
        glMultiTexCoord3fvARB(GL_TEXTURE0 + 1, vs);
#else
        glMultiTexCoord3fv(GL_TEXTURE0 + 1, vs);
#endif

        e->start->pos.getValues(vs); glVertex3fv(vs); 
      }
      e = e->next;
    } while( e != (*I)->edge);
  }
  glEnd();
  glPopAttrib();
}

bool TangentArrows::init()
{
  ShAttrib1f SH_DECL(scale) = .15;
  ShColor3f SH_DECL(color1) = ShColor3f(1.0, 0.0, 0.0);  
  ShColor3f SH_DECL(color2) = ShColor3f(0.0, 0.0, 1.0);
  
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputTexCoord1f tc;
    ShInputVector3f tangent;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputColor3f ocol; // Color of result
    
    ipos(0,1,2) += tc * scale * tangent;
    opos = Globals::mvp | ipos; // Compute NDC position
    ocol = lerp(tc, color2, color1);
  } SH_END;
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInOutColor3f col;
  } SH_END_PROGRAM;
  return true;
}


TangentArrows TangentArrows::instance = TangentArrows();


class NormalArrows : public Shader {
public:
  NormalArrows()
    : Shader("Debugging: Normal Arrows")
  {}
  
  ~NormalArrows() {}

  bool init();

  void render();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static NormalArrows instance;
};

void NormalArrows::render()
{

  glPushAttrib(GL_LINE_BIT);

  glLineWidth(2.0);
  
  float vs[4];
  glBegin(GL_LINES);
  const ShUtil::ShObjMesh& m_obj = *ShrikeCanvas::instance()->getModel();
  for(ShObjMesh::FaceSet::const_iterator I = m_obj.faces.begin();
      I != m_obj.faces.end(); ++I) {
    ShObjMesh::Edge* e = (*I)->edge;
    do {
      
      for (int i = 0; i < 2; i++) {
        e->normal.getValues(vs); glNormal3fv(vs);

        float f = (i ? 1.0 : 0.0);
#ifdef GL_ARB_multitexture
        glMultiTexCoord1fvARB(GL_TEXTURE0, &f);
#else
        glMultiTexCoord1fv(GL_TEXTURE0, &f);
#endif
      
        e->tangent.getValues(vs);
#ifdef GL_ARB_multitexture
	    glMultiTexCoord3fvARB(GL_TEXTURE0 + 1, vs);
#else
	    glMultiTexCoord3fv(GL_TEXTURE0 + 1, vs);
#endif
        
        e->start->pos.getValues(vs); glVertex3fv(vs); 
      }
      e = e->next;
    } while( e != (*I)->edge);
  }
  glEnd();
  glPopAttrib();
}

bool NormalArrows::init()
{
  ShAttrib1f SH_DECL(scale) = .15;
  ShColor3f SH_DECL(color1) = ShColor3f(1.0, 0.0, 0.0);  
  ShColor3f SH_DECL(color2) = ShColor3f(0.0, 0.0, 1.0);
  
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputTexCoord1f tc;
    ShInputVector3f tangent;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputColor3f ocol; // Color of result
    
    ipos(0,1,2) += tc * scale * inorm;
    opos = Globals::mvp | ipos; // Compute NDC position
    ocol = lerp(tc, color2, color1);
  } SH_END;
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInOutColor3f col;
  } SH_END_PROGRAM;
  return true;
}


NormalArrows NormalArrows::instance = NormalArrows();

