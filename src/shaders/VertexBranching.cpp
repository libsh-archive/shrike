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
#include <iostream>
#include "ShrikeGl.hpp"
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class VertexBranching : public Shader {
public:
  VertexBranching(const Globals&);
  ~VertexBranching();

  bool init();

  bool render(const ShObjMesh&);
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  GLuint displayList;
};

VertexBranching::VertexBranching(const Globals& globals)
  : Shader("Branching: Vertex Unit", globals),
    displayList(0)
{
}

VertexBranching::~VertexBranching()
{
}

bool VertexBranching::render(const ShObjMesh&)
{
  int divs = 500;

  if (!displayList) {
    displayList = glGenLists(1);

    glNewList(displayList, GL_COMPILE);
    glNormal3f(0.0, 0.0, 1.0);
    
    for (int y = 0; y < divs; y++) {
      glBegin(GL_QUAD_STRIP); {
        for (int x = 0; x < divs; x++) {
          float xpos = (float)x/(float)divs * 2.0 - 1.0;
          float ypos = (float)y/(float)divs * 2.0 - 1.0;
          float ypos2 = (float)(y+1)/(float)divs * 2.0 - 1.0;
          float xtc = (float)x/(float)divs - 0.5;
          float ytc = (float)y/(float)divs - 0.5;
          float ytc2 = (float)(y+1)/(float)divs - 0.5;
          
          glTexCoord2f(xtc, ytc);
          glVertex3f(xpos, ypos, 0.0f);
          glTexCoord2f(xtc, ytc2);
          glVertex3f(xpos, ypos2, 0.0f);
        }
      } glEnd();
    }
    glEndList();
  }

  glCallList(displayList);
  return true;
}

bool VertexBranching::init()
{
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(julia_max_iter) = 20.0;
  julia_max_iter.range(1.0, 40.0);
  ShAttrib2f SH_DECL(julia_c) = ShAttrib2f(0.54, -0.51);
  julia_c.range(-1.0, 1.0);
  ShAttrib1f SH_DECL(brightness) = 1.0;
  brightness.range(0.0, 10.0);
  ShAttrib1f SH_DECL(height) = 0.2;
  height.range(0.0, 4.0);

  ShAttrib1f SH_DECL(zoom) = 0.33;
  zoom.range(0.0, 10.0);

  ShPoint2f SH_DECL(centre) = ShPoint2f(0.0, 0.0);
  centre.range(-2.0, 2.0);
  
  ShAttrib1f SH_DECL(gamma) = 0.7;
  gamma.range(0.0, 1.0);
  
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputColor3f ocol; // Color of result

    ShInputTexCoord2f u;

    u = u - centre;
    u /= zoom;
    
    ShAttrib1f i = 0.0; 
    SH_WHILE((dot(u, u) < 2.0) * (i < julia_max_iter)) {
      ShTexCoord2f v;
      v(0) = u(0)*u(0) - u(1)*u(1);
      v(1) = 2.0 * u(0) * u(1);
      u = v + julia_c;
      i += 1.0;
    } SH_ENDWHILE;

    ShAttrib1f disp = pow(i / julia_max_iter, gamma);
    ocol = disp * diffuse * brightness;

    ipos(0,1,2) += disp * inorm * height;
    opos = m_globals.mvp | ipos; // Compute NDC position
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutColor3f color; // pass through the color.
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new VertexBranching(globals));
    return list;
  }
}
#else
static StaticLinkedShader<VertexBranching> instance = 
       StaticLinkedShader<VertexBranching>();
#endif
