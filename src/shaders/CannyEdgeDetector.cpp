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
#include <fstream>
#include "Shader.hpp"
#include "Globals.hpp"
#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glext.h>
#else
# define GL_GLEXT_LEGACY
# include <GL/gl.h>
# include <GL/glext.h>
# undef GL_GLEXT_LEGACY
#endif
#include "HDRImages/HDRImage.hpp"
#include "HDRImages/HDRInterp.hpp"
#include "HDRImages/Filters.hpp"

using namespace SH;
using namespace ShUtil;

class CannyEdgeDetectorShader : public Shader {
public:
  CannyEdgeDetectorShader();
  ~CannyEdgeDetectorShader();

  bool init();

  void render();
  
  ShProgram vertex() { return vsh_4th;}
  ShProgram fragment() { return fsh_4th;}

  ShProgram vsh_1st, fsh_1st, fsh_2nd, fsh_3rd, vsh_4th, fsh_4th;

  static CannyEdgeDetectorShader instance;

protected:
  ShHostMemoryPtr newmem;
  ShTextureRect<ShColor3f> newimg;

private:
  void renderPlane()
  {
    glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
    glBegin(GL_QUADS); { // just render a square with the texture on it
      glTexCoord2f(0.0, 0.0);
      glVertex2f(-1.0, -1.0);
      glTexCoord2f(0.0, 1.0);
      glVertex2f(-1.0, 1.0);
      glTexCoord2f(1.0, 1.0);
      glVertex2f(1.0, 1.0);
      glTexCoord2f(1.0, 0.0);
      glVertex2f(1.0, -1.0);
    }
    glEnd(); 
    glFlush();
  }
  int vp[4];
};

CannyEdgeDetectorShader::CannyEdgeDetectorShader()
  : Shader("Canny Edge Detector")
{
}

CannyEdgeDetectorShader::~CannyEdgeDetectorShader()
{
}

void CannyEdgeDetectorShader::render()
{
  int newvp[4];
  glGetIntegerv(GL_VIEWPORT, newvp);
  // check if the texture size has to be changed
  if(newvp[0] != vp[0] || newvp[1] != vp[1] || newvp[2] != vp[2] || newvp[3] != vp[3]) {
    for(int i=0 ; i<4 ; i++)
      vp[i] = newvp[i]; // set the new size
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    // 1st pass
    shBind(vsh_1st);
    shBind(fsh_1st);
    renderPlane();
    newmem->hostStorage()->dirty(); // needed to update the value after
    newmem = new ShHostMemory((vp[2]-vp[0])*(vp[3]-vp[1])*3*sizeof(float)); // allocate memory
    newimg.size(vp[2]-vp[0],vp[3]-vp[1]); // change the size
    newimg.memory(newmem);
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_FLOAT, newmem->hostStorage()->data()); // read the buffer
    glReadBuffer(GL_FRONT);
    // 2nd pass
    shBind(fsh_2nd);
    renderPlane();
    newmem->hostStorage()->dirty(); // needed to update the value after
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_FLOAT, newmem->hostStorage()->data()); // read the buffer
    glReadBuffer(GL_FRONT);
    // 3rd pass
    shBind(fsh_3rd);
    renderPlane();
    newmem->hostStorage()->dirty(); // needed to update the value after
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_FLOAT, newmem->hostStorage()->data()); // read the buffer
    glReadBuffer(GL_FRONT);
  }
  // 4th pass
  shBind(vsh_4th);
  shBind(fsh_4th);
  glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
  Shader::render();
}

bool CannyEdgeDetectorShader::init()
{
  for(int i=0 ; i<4 ; i++)
    vp[i] =0;
  HDRImage image;
  std::string filename = SHMEDIA_DIR "/hdr/hdr/memorial.hdr";
  image.loadHDR(filename.c_str());
  AnisDiff<CatmullRomInterp<ShTextureRect<ShVector4f> > > img(image.width(), image.height());
  img.internal(true);
  img.memory(image.memory());
  newmem = new ShHostMemory(3*sizeof(float)); // the real size will be specified in render()
  newimg.internal(true);
  newimg.memory(newmem);
    
  vsh_1st = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInOutPosition4f ipos; // no change
    ShInOutTexCoord2f tc;
  } SH_END;
  
  vsh_4th = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShOutputPosition4f opos;
    ShInOutTexCoord2f tc;
    opos = Globals::mvp | ipos;
  } SH_END;

  fsh_1st = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShOutputColor3f result;
    ShAttrib3f RGB = 5.55555098 * img(u)(0,1,2);
    ShAttrib1f f = 0.184874;
    ShAttrib1f e = 2.718281828;
    RGB(0) = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
    RGB(1) = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
    RGB(2) = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
    ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
    result = pow(RGB,gammainv(0,0,0)) * 0.285714286;
  } SH_END;

  fsh_2nd = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShOutputColor3f result; 
      
    u *= newimg.size();
    ShAttrib3f lumvect(0.27,0.67,0.06); // to compute the luminance
    /*
      ShAttrib1f Gx = lumvect | (newimg[u] - newimg[u+ShAttrib2f(1.0,1.0)]);
      ShAttrib1f Gy = lumvect | (newimg[u+ShAttrib2f(1.0,0.0)] - newimg[u+ShAttrib2f(0.0,1.0)]);
    */
    ShAttrib1f Gx = lumvect | (2.0*newimg[u+ShAttrib2f(1.0,0.0)] + newimg[u+ShAttrib2f(1.0,1.0)] + newimg[u+ShAttrib2f(1.0,-1.0)] -
			       2.0*newimg[u-ShAttrib2f(1.0,0.0)] - newimg[u-ShAttrib2f(1.0,1.0)] - newimg[u-ShAttrib2f(1.0,-1.0)]);            
    ShAttrib1f Gy = lumvect | (2.0*newimg[u-ShAttrib2f(0.0,1.0)] + newimg[u-ShAttrib2f(1.0,1.0)] + newimg[u-ShAttrib2f(-1.0,1.0)] -
			       2.0*newimg[u+ShAttrib2f(0.0,1.0)] - newimg[u+ShAttrib2f(1.0,1.0)] - newimg[u+ShAttrib2f(-1.0,1.0)]);            
    ShAttrib1f G = sqrt(Gx*Gx + Gy*Gy);
    result = ShColor3f(Gx,Gy,G);
  } SH_END;

  fsh_3rd = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShOutputColor3f result;
    u *= newimg.size();
    result = newimg[u];
    ShAttrib1f frac = abs(result(1)/result(0));
    ShAttrib1f edgeDir = cond(abs(result(0))>ShAttrib1f(0.01), 0.5*asin(2.0*frac/(frac*frac + 1.0)),
			      cond(abs(result(1))<ShAttrib1f(0.01), ShAttrib1f(0.0), ShAttrib1f(M_PI/2.0)));
    edgeDir *= 180.0/M_PI;
    edgeDir = cond(result(0)*result(1)<0.0, 180.0-edgeDir, edgeDir);
    ShAttrib1f G = result(2);
    ShAttrib1f angle;
    angle = SH::max(edgeDir > 157.5 , edgeDir < 22.5);
    G = cond(SH::min(SH::max(newimg[u+ShAttrib2f(1.0,0.0)](2) > G, newimg[u-ShAttrib2f(1.0,0.0)](2) > G), angle), ShAttrib1f(0.0) , G);
    angle = SH::min(edgeDir > 22.5, edgeDir < 67.5);
    G = cond(SH::min(SH::max(newimg[u+ShAttrib2f(1.0,1.0)](2) > G, newimg[u-ShAttrib2f(1.0,1.0)](2) > G), angle), ShAttrib1f(0.0) , G);
    angle = SH::min(edgeDir > 67.5, edgeDir < 112.5);
    G = cond(SH::min(SH::max(newimg[u+ShAttrib2f(0.0,1.0)](2) > G, newimg[u-ShAttrib2f(0.0,1.0)](2) > G), angle), ShAttrib1f(0.0) , G);
    angle = SH::min(edgeDir > 112.5, edgeDir < 157.5);
    G = cond(SH::min(SH::max(newimg[u+ShAttrib2f(-1.0,1.0)](2) > G, newimg[u-ShAttrib2f(-1.0,1.0)](2) > G), angle), ShAttrib1f(0.0) , G);
    result = G(0,0,0);
  } SH_END;
   
   
  ShAttrib1f SH_DECL(hysteresis1) = ShAttrib1f(0.07);
  ShAttrib1f SH_DECL(hysteresis2) = ShAttrib1f(0.09);

  fsh_4th = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShOutputColor3f result;
    u *= newimg.size();
    ShAttrib1f G = newimg[u](0);
    ShAttrib1f G1 = newimg[u+ShAttrib2f(0.0,1.0)](0);
    ShAttrib1f G2 = newimg[u-ShAttrib2f(0.0,1.0)](0);
    ShAttrib1f G3 = newimg[u+ShAttrib2f(1.0,1.0)](0);
    ShAttrib1f G4 = newimg[u-ShAttrib2f(1.0,1.0)](0);
    ShAttrib1f G5 = newimg[u+ShAttrib2f(1.0,0.0)](0);
    ShAttrib1f G6 = newimg[u-ShAttrib2f(1.0,0.0)](0);
    ShAttrib1f G7 = newimg[u+ShAttrib2f(-1.0,1.0)](0);
    ShAttrib1f G8 = newimg[u-ShAttrib2f(-1.0,1.0)](0);
    ShAttrib1f hysteresis = SH::max(SH::max(SH::max(SH::max(SH::max(SH::max(SH::max(G1>hysteresis2, G2>hysteresis2), G3>hysteresis2), G4>hysteresis2), G4>hysteresis2), G6>hysteresis2), G7>hysteresis2), G8>hysteresis2);
    G = cond(G>hysteresis1, cond(SH::max(G>hysteresis2, hysteresis), G, ShAttrib1f(0.0)), ShAttrib1f(0.0)) ;
    result = G(0,0,0);
  } SH_END;
  
  return true;
}

CannyEdgeDetectorShader CannyEdgeDetectorShader::instance = CannyEdgeDetectorShader();
