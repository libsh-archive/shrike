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
#if defined( __APPLE__ )
#define GL_GLEXT_VERBOSE 1
#define GL_GLEXT_PROTOTYPES 1
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#endif
#include <sh/ShObjMesh.hpp>

using namespace SH;
using namespace ShUtil;
using namespace std;

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
  ShHostMemoryPtr newmem;
  ShTextureRect<ShColor3f> img;
  int vp[4];
};

EdgeDetection::EdgeDetection()
  : Shader("Edge Detection: Object Detection "), fname("triceratops.obj")
{
	setStringParam("Object", fname);
}

EdgeDetection::~EdgeDetection()
{
}

void EdgeDetection::render()
{
  int newvp[4];
	glGetIntegerv(GL_VIEWPORT, newvp); // get the size to adapte the texture size after
	shBind(vsh_model);
	shBind(fsh_model); // just render white
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
  float values[4];
  glBegin(GL_TRIANGLES); // render the object
  for(ShObjMesh::FaceSet::iterator I = m_model->faces.begin(); I != m_model->faces.end(); ++I) {
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
  glFlush();
  newmem->hostStorage()->dirty(); // needed to update the value after
  if(newvp[0] != vp[0] || newvp[1] != vp[1] || newvp[2] != vp[2] || newvp[3] != vp[3]) // check if the texture size has to be changed
  {
    for(int i=0 ; i<4 ; i++)
      vp[i] = newvp[i]; // set the new size
    newmem = new ShHostMemory((vp[2]-vp[0])*(vp[3]-vp[1])*3*sizeof(float)); // allocate memory
    img.size(vp[2]-vp[0],vp[3]-vp[1]); // change the size
    img.memory(newmem);
  }
  glReadBuffer(GL_BACK);
	glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_FLOAT, newmem->hostStorage()->data()); // read the buffer
  glReadBuffer(GL_FRONT);
	shBind(vsh_edge);
	shBind(fsh_edge); // edge detection
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT); // to avoid the object(s) already rendered
  
	glBegin(GL_QUADS); { // just render a squaree with the texture
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
  for(int i=0 ; i<4 ; i++)
    vp[i] = 0;
	newmem = new ShHostMemory(3*sizeof(float)); // the real size will be specified in render()
	img.memory(newmem);
		
	vsh_model = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
		ShOutputPosition4f opos;
    ShInOutTexCoord2f tc;
		opos = Globals::mvp | ipos;
  } SH_END;
	
  vsh_edge = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInOutPosition4f ipos; // no change
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


