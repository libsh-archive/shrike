#include <iostream>
#include <fstream>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <GL/gl.h>
#include "Shader.hpp"
#include "Globals.hpp"
#include "Text.hpp"
#include "config.h"
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
  ShProgram fragment() { return fsh_h & fsh_s;}
  
  ShProgram vsh;
  ShProgram fsh_h, fsh_s;

private:

  ShPoint3f saw, sbw;

  ShObjMesh* m_model;
  
  static Logo* instance;
};

Logo::Logo()
  : Shader("Sh Logo")
{
  saw = ShPoint3f(0, 1.35, 0);
  sbw = ShPoint3f(1, 1.35, 0);
  saw.name("saw");
  sbw.name("sbw");
}

Logo::~Logo()
{
}

void Logo::bind()
{
  shBindShader(vsh);
}

void Logo::render()
{
  shBindShader(fsh_h);

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

  shBindShader(fsh_s);

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
    ShPoint3f l;
    ShPoint2f a, b;
    ShInputTexCoord2f i;
    ShOutputTexCoord2f o;

    ShPoint3f lightPos = Globals::lightDirW * Globals::lightLenW;
    
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
  
  ShProgram s_renderer = lose<ShTexCoord2f>() & ShKernelLib::shDiffuse<ShColor3f>() << color;

  fsh_s = s_renderer;
  fsh_h = h_renderer << doText("h") << scaler_h << warper;
  
  return true;
}

Logo* Logo::instance = new Logo();