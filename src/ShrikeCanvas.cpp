#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include <wx/glcanvas.h>
#include "ShrikeCanvas.hpp"
#include "Globals.hpp"
#include "ShTrackball.hpp"
#include <sh/sh.hpp>

using namespace SH;
using namespace ShUtil;

BEGIN_EVENT_TABLE(ShrikeCanvas, wxGLCanvas)
  EVT_PAINT(ShrikeCanvas::paint)
  EVT_SIZE(ShrikeCanvas::reshape)
  EVT_MOTION(ShrikeCanvas::motion)
END_EVENT_TABLE()

ShrikeCanvas* ShrikeCanvas::m_instance = 0;
  
ShrikeCanvas::ShrikeCanvas(wxWindow* parent, ShObjMesh* model)
  : wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize),
    m_init(false),
    m_model(model),
    m_shader(0),
    m_showLight(true)
{
  m_instance = this;
  Globals::mv.internal(true);
  Globals::mvp.internal(true);
  Globals::lightPos.internal(true);
  Globals::lightDirW.internal(true);
  Globals::lightLenW.internal(true);
  resetView();
}

ShrikeCanvas* ShrikeCanvas::instance()
{
  return m_instance;
}

void ShrikeCanvas::paint()
{
  render();
}

void ShrikeCanvas::setModel(ShObjMesh* model)
{
  if (m_model == model) return;
  delete m_model;
  m_model = model;
  render();
}

void ShrikeCanvas::setShader(Shader* shader)
{
  SetCurrent();
  if (shader) {
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
  } else {
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
  }
  m_shader = shader;
}

void ShrikeCanvas::motion(wxMouseEvent& event)
{
  if (!event.Dragging()) {
    m_last_x = event.GetX();
    m_last_y = event.GetY();
    return;
  }

  long cur_x = event.GetX();
  long cur_y = event.GetY();
  long dx = cur_x - m_last_x;
  long dy = cur_y - m_last_y;

  if (event.LeftIsDown()) {
    if (event.ShiftDown()) {
      ShTrackball t;
      t.resize(m_width, m_height);
      Globals::lightDirW = inv(Globals::mv) | t.rotate(m_last_x, m_last_y, cur_x, cur_y) | Globals::mv | Globals::lightDirW;
    } else {
      m_camera.orbit(m_last_x, m_last_y, cur_x, cur_y, m_width, m_height);
    }
  }
  if (event.MiddleIsDown()) {
    if (event.ShiftDown()) {
      Globals::lightLenW += dy/10.0;
    } else {
      m_camera.move(0.0, 0.0, dy/3.0);
    }
  }
  if (event.RightIsDown()) {
    m_camera.move(dx/30.0, -dy/30.0, 0.0);
  }
  
  setupView();
  render();
  m_last_x = event.GetX();
  m_last_y = event.GetY();
}

void ShrikeCanvas::render()
{
  SetCurrent();
  init();
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_shader) m_shader->render();

  glDisable(GL_FRAGMENT_PROGRAM_ARB);
  glDisable(GL_VERTEX_PROGRAM_ARB);

  ShPoint3f lp = Globals::lightDirW * Globals::lightLenW;
  float pos[3];
  lp.getValues(pos);

  glPointSize(3.0);
  
  glBegin(GL_POINTS); {
    glColor3f(1.0, 0.0, 1.0);
    glVertex3fv(pos);
  } glEnd();

  if (m_shader) {
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glEnable(GL_VERTEX_PROGRAM_ARB);
  }
  
  glFlush();
  
  SwapBuffers();
}

void ShrikeCanvas::renderObject()
{

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

void ShrikeCanvas::setupView()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  m_camera.glProjection((float)m_width/m_height);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  m_camera.glModelView();

  Globals::mv = m_camera.shModelView();
  Globals::mv_inverse = inv(Globals::mv);
  Globals::mvp = m_camera.shModelViewProjection(ShMatrix4x4f());
  Globals::lightPos = Globals::mv | ShPoint3f(Globals::lightDirW * Globals::lightLenW);
}

void ShrikeCanvas::init()
{
  if (m_init) return;

  glEnable(GL_DEPTH_TEST);

  glClearColor(0.2, 0.2, 0.2, 1.0);
  setupView();
  
  shSetBackend("arb");
  
  m_init = true;
}

void ShrikeCanvas::reshape()
{
  if (GetContext()) {
    SetCurrent();
    glViewport(0, 0, m_width, m_height);
    setupView();
  }
}

void ShrikeCanvas::resetView()
{
  m_camera = Camera();
  m_camera.move(0, 0.0, -7.0);
  if (GetContext()) {
    SetCurrent();
    setupView();
    render();
  }
}
