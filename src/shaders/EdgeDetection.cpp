// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
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
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include <sh/ShObjMesh.hpp>

using namespace SH;
using namespace ShUtil;


class EdgeDetection : public Shader {
public:
  EdgeDetection();
  ~EdgeDetection();

  bool init();

	void render();
  
  ShProgram vertex() { return vsh_edge;}
  ShProgram fragment() { return fsh_edge;}

  ShProgram vsh_edge, fsh_edge, vsh_model, fsh_model;

  static EdgeDetection instance;

	std::string fname;
	
private:
	ShObjMesh* m_model;
  unsigned int  _iTexture; 
};

EdgeDetection::EdgeDetection()
  : Shader("EdgeDetection"), fname("triceratops.obj")
{
	setStringParam("Object", fname);
}

EdgeDetection::~EdgeDetection()
{
}

void EdgeDetection::render()
{
	// render the object in a buffer
	shBind(vsh_model);
	shBind(fsh_model);
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	glViewport(0, 0, 512,512);
	glClear(GL_COLOR_BUFFER_BIT);
  float values[4];
  glBegin(GL_TRIANGLES);
  for(ShObjMesh::FaceSet::iterator I = m_model->faces.begin();
      I != m_model->faces.end(); ++I) {
    ShObjEdge *e = (*I)->edge;
    do {
      e->normal.getValues(values); 
      glNormal3fv(values);

      e->texcoord.getValues(values);
      glMultiTexCoord2fvARB(GL_TEXTURE0, values);

      e->tangent.getValues(values);
      glMultiTexCoord2fvARB(GL_TEXTURE0 + 1, values);

      e->start->pos.getValues(values);
      glVertex3fv(values);
      e = e->next;
    } while(e != (*I)->edge);
  }
	glEnd();
	glBindTexture(GL_TEXTURE_2D, _iTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 512, 512); // copy the buffer to a texture
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	// render the texture
	shBind(vsh_edge);
	shBind(fsh_edge);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, _iTexture);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS); { // just render a square with the texture
	  glTexCoord2f(0.0, 0.0);
	  glVertex2f(-1.0, -1.0);
	  glTexCoord2f(0.0, 1.0);
	  glVertex2f(-1.0, 1.0);
	  glTexCoord2f(1.0, 1.0);
	  glVertex2f(1.0, 1.0);
	  glTexCoord2f(1.0, 0.0);
	  glVertex2f(1.0, -1.0);
	} glEnd();
	glDisable(GL_TEXTURE_2D);

}

bool EdgeDetection::init()
{
	std::string objFile = SHMEDIA_DIR "/objs/" + fname;
	std::ifstream infile(objFile.c_str());
  if (infile) {
    m_model = new ShObjMesh(infile);
  } else {
    return false;
  }

	ShTexture2D<ShColor3f> img(512,512);
	ShHostMemoryPtr newmem = new ShHostMemory(512*512*3*sizeof(float));
	img.memory(newmem);
		
	glGenTextures(1, &_iTexture);
	glBindTexture(GL_TEXTURE_2D, _iTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGB, GL_FLOAT, newmem->hostStorage()->data());
  
	vsh_model = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
		ShOutputPosition4f opos;
    ShInOutTexCoord2f tc;
		opos = Globals::mvp | ipos;
  } SH_END;
	
  vsh_edge = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInOutPosition4f ipos;
    ShInOutTexCoord2f tc;
  } SH_END;

	fsh_model = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
		ShOutputColor3f resut(1.0,1.0,1.0);
	} SH_END;

  fsh_edge = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
     
    ShOutputColor3f result;
		
		ShAttrib2f offset1(0.001953125,0.0);
		ShAttrib2f offset2(0.0,0.001953125);
		
		result = 8.0 * (img(u) - 0.125 *(img(u-offset1-offset2) + 
																		 img(u-offset1) +
																		 img(u-offset1+offset2) +
																		 img(u+offset2) +
																		 img(u+offset1-offset2) +
																		 img(u-offset1) +
																		 img(u+offset1+offset2) +
																		 img(u-offset2)));
	} SH_END;
  return true;
}

EdgeDetection EdgeDetection::instance = EdgeDetection();


