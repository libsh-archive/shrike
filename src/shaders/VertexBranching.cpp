#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class VertexBranching : public Shader {
public:
  VertexBranching();
  ~VertexBranching();

  bool init();

  void render();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static VertexBranching instance;
};

VertexBranching::VertexBranching()
  : Shader("Branching: Vertex Unit")
{
}

VertexBranching::~VertexBranching()
{
}

void VertexBranching::render()
{
  int divs = 1000;

  glBegin(GL_POINTS); {
    for (int y = 0; y < divs; y++) for (int x = 0; x < divs; x++) {
      float xpos = (float)x/(float)divs * 2.0 - 1.0;
      float ypos = (float)y/(float)divs * 2.0 - 1.0;
      float xtc = (float)x/(float)divs - 0.5;
      float ytc = (float)y/(float)divs - 0.5;

      glTexCoord2f(xtc, ytc);
      glNormal3f(0.0, 0.0, 1.0);
      glVertex3f(xpos, ypos, 0.0f);
    }
  } glEnd();
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
    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputColor3f color;
    ShInputPosition4f posh;

    ShOutputColor3f result = color;
  } SH_END;
  return true;
}

VertexBranching VertexBranching::instance = VertexBranching();

