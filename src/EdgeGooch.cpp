#include <iostream>
#include <fstream>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include "Shader.hpp"
#include "Globals.hpp"
#include "config.h"
#include "ShrikeCanvas.hpp"
#include <sh/ShObjMesh.hpp>

using namespace SH;
using namespace ShUtil;

class EdgeGooch : public Shader {
public:
  EdgeGooch();
  ~EdgeGooch();

  bool init();
  void initLists();

  void bind();
  
  void render();
  void facemodeVert(const ShObjEdge *e0, const ShObjEdge* e1,
    const ShObjEdge *e2, float move); 
  
  ShProgram vertex() { return vsh & edgevsh;}
  ShProgram fragment() { return fsh & edgefsh; } 
  
  ShProgram vsh, edgevsh;
  ShProgram fsh, edgefsh; 

private:
  ShObjMesh m_obj;
  const ShObjMesh *m_canvasobj; 
  ShPoint3f eyePosm;
  ShAttrib1f aspect;
  GLuint displayList;
  GLuint edgeDisplayList; 
  
  static EdgeGooch* instance;
};

EdgeGooch::EdgeGooch()
  : Shader("Sillhouetted Gooch") { 
    m_canvasobj = 0;
}

EdgeGooch::~EdgeGooch() { }

void EdgeGooch::bind() { }

void EdgeGooch::render() {
  const ShrikeCanvas *canvas = ShrikeCanvas::instance();
  if( m_canvasobj != canvas->getModel()) {
    m_canvasobj = canvas->getModel(); 
    m_obj = *m_canvasobj; 
    m_obj.consolidateVertices();
    m_obj.mergeEdges();
    m_obj.normalizeNormals();
    initLists();
  }
  aspect = canvas->m_width / (float)canvas->m_height; 

  // render fill 
  shBindShader(vsh);
  shBindShader(fsh);
  glCallList(displayList);

  eyePosm = Globals::mv_inverse | ShPoint3f(0, 0, 0);
  shBindShader(edgevsh);
  shBindShader(edgefsh);
  glCallList(edgeDisplayList);
}

bool EdgeGooch::init()
{
  vsh = ShKernelLib::shVsh(Globals::mv, Globals::mvp);
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  vsh = shSwizzle("normal", "lightVec", "posh") << vsh;

  ShColor3f SH_NAMEDECL(diffuseColor, "Diffuse Color") = ShConstAttrib3f(0.9f, 0.9f, 0.6f);
  ShColor3f SH_NAMEDECL(cool, "Cool Color") = ShConstAttrib3f(0.2f, 0.2f, 1.0f);
  ShColor3f SH_NAMEDECL(warm, "Warm Color") = ShConstAttrib3f(1.0f, 0.3f, 0.3f);
  ShAttrib1f SH_NAMEDECL(width, "Edge Width") = ShConstAttrib1f(0.02f);
  width.range(0.001f, 0.5f);
  ShAttrib1f SH_NAMEDECL(poffset, "Polygon Offset") = ShConstAttrib1f(0.0f);
  poffset.range(-1.0f, 1.0f);


  fsh = SH_BEGIN_PROGRAM("gpu:fragment") { // gooch shader
    ShInputNormal3f SH_DECL(normal);
    ShInputVector3f SH_DECL(lightVec);

    normal = normalize(normal);
    lightVec= normalize(lightVec);
    ShConstAttrib1f half(0.5f);
    ShAttrib1f blend = mad(dot(normal, lightVec), half, half);
    ShOutputColor3f SH_DECL(result) = diffuseColor * lerp(blend, warm, cool);
  } SH_END;

  edgevsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInOutNormal3f SH_NAMEDECL(n0, "normal"); // MCS normal (n0)
    ShInOutTexCoord2f SH_NAMEDECL(tc0, "texcoord" );
    ShInputTexCoord2f SH_DECL(tc1);
    ShInputTexCoord2f SH_DECL(tc2);
    ShInOutAttrib1f SH_DECL(move); // 

    ShInputPoint3f  SH_DECL(p1);
    ShInputNormal3f  SH_DECL(n1);

    ShInputPoint3f  SH_DECL(p2);
    ShInputNormal3f  SH_DECL(n2);

    ShInOutPosition4f SH_NAMEDECL(p0, "posm");

    ShAttrib3f nvv; // nvv(i) holds dot product of ni and view vec to pi
    nvv(0) = normalize(eyePosm - p0(0,1,2)) | n0;
    nvv(1) = normalize(eyePosm - p1) | n1;
    nvv(2) = normalize(eyePosm - p2) | n2;

    // s(i) = 1 if there is a sign change between ni, ni+1
    ShAttrib3f s = (nvv * nvv(1,2,0)) < 0.0f;

    // find interpolated points between p0,p1 and p0,p2
    ShAttrib2f k = -nvv(1,2) / (nvv(0,0) - nvv(1,2));
    ShAttrib2f nk = ShConstAttrib2f(1.0f, 1.0f) - k;
    p1 = lerp(k(0), p0(0,1,2), p1);
    n1 = lerp(k(0), n0, n1);
    tc1 = lerp(k(0), tc0, tc1);
    p2 = lerp(k(1), p0(0,1,2), p2);
    n2 = lerp(k(1), n0, n2); 
    tc2 = lerp(k(1), tc0, tc2); 

    // pick which interpolated point to use based on conditions
    ShAttrib1f snone = dot(s, s) < 0.5f; // not on sillhouete
    ShAttrib1f nots2_and_s0 = (1.0f - s(2)) * s(0); // 
    p0(0,1,2) = snone * p0(0,1,2) + s(2) * p2 + nots2_and_s0 * p1;
    n0 = snone * n0 + s(2) * n2 + nots2_and_s0 * n1;
    tc0 = snone * tc0 + s(2) * tc2 + nots2_and_s0 * tc1;

    move *= ( 1.0f - snone ); // only expand quad on sillhouette 
  } SH_END;

  ShProgram translator = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f SH_DECL(posm); // in MCS

    ShInOutTexCoord2f SH_DECL(texcoord);
    ShInOutNormal3f SH_DECL(normal); // in MCS, out VCS
    ShInputAttrib1f SH_DECL(move); // whether to move 
    ShOutputPosition4f SH_DECL(posh); // out DCS

    posh = Globals::mvp | posm;

    ShPoint3f offposh = posm(0,1,2) + normal;
    offposh = Globals::mvp | offposh;
    ShPoint3f nph = posh(0,1,2) / posh(3);
    ShVector2f offset = normalize(offposh - nph )(0,1) * width * move * posh(3);
    offset(1) *= aspect;

    posh(0,1) += offset;
    posh(2) += poffset;

    normal = normalize(Globals::mv | normal);
    texcoord(0) += texcoord(1);
    texcoord(1) = move;
  } SH_END;

  edgevsh = namedConnect( edgevsh, translator );

  edgefsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShOutputColor4f SH_DECL(kd) = ShConstAttrib4f(0.0f, 0.0f, 0.0f, 1.0f);
  } SH_END;

  return true;
}

void EdgeGooch::facemodeVert(const ShObjEdge *e0, const ShObjEdge* e1,
    const ShObjEdge *e2, float move) {
  // Note, because of current backend binding order, texcoords must go first
  float vs[4];
  vs[3] = 0;
  e0->normal.getValues(vs); glNormal3fv(vs);
  e0->texcoord.getValues(vs); glMultiTexCoord2fvARB(GL_TEXTURE0, vs);
  e1->texcoord.getValues(vs); glMultiTexCoord2fvARB(GL_TEXTURE0 + 1, vs);
  e2->texcoord.getValues(vs); glMultiTexCoord2fvARB(GL_TEXTURE0 + 2, vs);
  glMultiTexCoord1fARB(GL_TEXTURE0 + 3, move);
  e1->start->pos.getValues(vs); glMultiTexCoord3fvARB(GL_TEXTURE0 + 4, vs);
  e1->normal.getValues(vs); glMultiTexCoord3fvARB(GL_TEXTURE0 + 5, vs);
  e2->start->pos.getValues(vs); glMultiTexCoord3fvARB(GL_TEXTURE0 + 6, vs);
  e2->normal.getValues(vs); glMultiTexCoord3fvARB(GL_TEXTURE0 + 7, vs);
  e0->start->pos.getValues(vs); glVertex3fv(vs);
}

void EdgeGooch::initLists() {
  float vs[4];

  displayList = glGenLists(1);
  glNewList(displayList, GL_COMPILE);
  glBegin(GL_TRIANGLES);
    for(ShObjMesh::FaceSet::iterator I = m_obj.faces.begin();
          I != m_obj.faces.end(); ++I) {
      ShObjMesh::Edge* e = (*I)->edge;
      do {
        e->normal.getValues(vs); glNormal3fv(vs);
        e->texcoord.getValues(vs); glTexCoord2fv(vs);
        e->start->pos.getValues(vs); glVertex3fv(vs); 
        e = e->next;
      } while( e != (*I)->edge);
    }
  glEnd();
  glEndList();


  ShObjMesh fixedObj = m_obj;
  fixedObj.generateVertexNormals(true); // force generate a single normal per vertex
  fixedObj.normalizeNormals();

  edgeDisplayList = glGenLists(1);
  glNewList(edgeDisplayList, GL_COMPILE);

  glBegin(GL_QUADS);
  for(ShObjMesh::FaceSet::iterator I = fixedObj.faces.begin();
      I != fixedObj.faces.end(); ++I) {
    ShObjMesh::Edge *e0, *e1, *e2;
    e0 = (*I)->edge;
    e1 = e0->next;
    e2 = e1->next;

    // TODO cannot handle non-triangular faces yet
    // (so make sure to triangulate the mesh)
    SH_DEBUG_ASSERT( e2->next == e0 );
    facemodeVert(e0, e1, e2, 0.0f);
    facemodeVert(e1, e0, e2, 0.0f);
    facemodeVert(e1, e0, e2, 1.0f);
    facemodeVert(e0, e1, e2, 1.0f);
  }
  glEnd();
  glEndList();
}

EdgeGooch* EdgeGooch::instance = new EdgeGooch();
