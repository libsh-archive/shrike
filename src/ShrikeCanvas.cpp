#include <GL/gl.h>
#include <GL/glext.h>
#include <wx/glcanvas.h>
#include "ShrikeCanvas.hpp"
#include "Globals.hpp"

using namespace SH;

BEGIN_EVENT_TABLE(ShrikeCanvas, wxGLCanvas)
  EVT_PAINT(ShrikeCanvas::paint)
  EVT_SIZE(ShrikeCanvas::reshape)
  EVT_MOTION(ShrikeCanvas::motion)
END_EVENT_TABLE()

ShrikeCanvas* ShrikeCanvas::m_instance = 0;
  
ShrikeCanvas::ShrikeCanvas(wxWindow* parent, ShObjFile* model)
  : wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize),
    m_init(false),
    m_model(model)
{
  m_camera.move(0, 0.0, -7.0);
  m_instance = this;
  Globals::mv.internal(true);
  Globals::mvp.internal(true);
  Globals::lightPos.internal(true);
  Globals::lightPosW.internal(true);
}

ShrikeCanvas* ShrikeCanvas::instance()
{
  return m_instance;
}

void ShrikeCanvas::paint()
{
  render();
}

void ShrikeCanvas::setModel(ShObjFile* model)
{
  if (m_model == model) return;
  delete m_model;
  m_model = model;
  render();
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
    m_camera.orbit(m_last_x, m_last_y, cur_x, cur_y, m_width, m_height);
  }
  if (event.MiddleIsDown()) {
    m_camera.move(0.0, 0.0, dy/3.0);
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
  
  glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);

  glBegin(GL_TRIANGLES);
  for(std::size_t i = 0; i < m_model->faces.size(); i++) {
    float values[3];
    for (int j = 0; j < 3; j++) {
      if (m_model->normals.size() && m_model->faces[i].normals[j] >= 0) {
        m_model->normals[m_model->faces[i].normals[j]].getValues(values);
        glNormal3fv(values);
      }
      
      if (m_model->texcoords.size() && m_model->faces[i].texcoords[j] >= 0) {
        m_model->texcoords[m_model->faces[i].texcoords[j]].getValues(values);
        glMultiTexCoord2fvARB(0, values);
      }

      if (m_model->tangents.size() && m_model->faces[i].tangents[j] >= 0) {
        m_model->tangents[m_model->faces[i].tangents[j]].getValues(values);
        glMultiTexCoord3fvARB(1, values);
      }

      m_model->vertices[m_model->faces[i].points[j]].getValues(values);
      glVertex3fv(values);
    }
  }
  glEnd();

  glFlush();
  
  SwapBuffers();
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
  Globals::mvp = m_camera.shModelViewProjection(ShMatrix4x4f());
  Globals::lightPos = Globals::mv | Globals::lightPosW;
}

void ShrikeCanvas::init()
{
  if (m_init) return;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_VERTEX_PROGRAM_ARB);
  glEnable(GL_FRAGMENT_PROGRAM_ARB);

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
