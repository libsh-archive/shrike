#include <sstream>
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include <wx/glcanvas.h>
#include "ShrikeCanvas.hpp"
#include "ShrikeFrame.hpp"
#include "Globals.hpp"
#include "ShTrackball.hpp"
#include <sh/sh.hpp>

using namespace SH;
using namespace ShUtil;

BEGIN_EVENT_TABLE(ShrikeCanvas, wxGLCanvas)
  EVT_PAINT(ShrikeCanvas::paint)
  EVT_SIZE(ShrikeCanvas::reshape)
  EVT_MOTION(ShrikeCanvas::motion)
  EVT_KEY_DOWN(ShrikeCanvas::keyDown)
END_EVENT_TABLE()

ShrikeCanvas* ShrikeCanvas::m_instance = 0;
  
ShrikeCanvas::ShrikeCanvas(wxWindow* parent, ShObjMesh* model)
  : wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize),
    m_init(false),
    m_model(model),
    m_model_dirty(true),
    m_model_list(0),
    m_shader(0),
    m_showLight(true),
    m_bg_r(0.2), m_bg_g(0.2), m_bg_b(0.2)
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
  m_model_dirty = true;
  render();
}

const ShObjMesh* ShrikeCanvas::getModel() const {
  return m_model;
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
  
  glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);

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
  if (m_model_list == 0) {
    m_model_list = glGenLists(1);
  }

  if (m_model_dirty) {
    glNewList(m_model_list, GL_COMPILE_AND_EXECUTE);
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
    glEndList();
    m_model_dirty = false;
  } else {
    glCallList(m_model_list);
  }
}

void ShrikeCanvas::setupView(int nsplit, int x, int y)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  ShMatrix4x4f split;
  
  if (nsplit > 1) {
    split[0][0] = 2.0;
    split[1][1] = 2.0;
    split[2][2] = 1.0;
    split[3][3] = 1.0;
    split[0][3] = (float)(1-x*2);
    split[1][3] = (float)(1-y*2);
    float values[16];
    for (int i = 0; i < 16; i++) split[i%4](i/4).getValues(&values[i]);
    glMultMatrixf(values);
  }
  
  m_camera.glProjection((float)m_width/m_height);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  m_camera.glModelView();

  Globals::mv = m_camera.shModelView();
  Globals::mv_inverse = inv(Globals::mv);
  Globals::mvp = m_camera.shModelViewProjection(split);
  Globals::lightPos = Globals::mv | ShPoint3f(Globals::lightDirW * Globals::lightLenW);
}

void ShrikeCanvas::screenshot(const std::string& filename)
{
  ShImage final(m_width*2, m_height*2, 3);
  float* fd = final.data();
    
  for (int y = 0; y < 2; y++) for (int x = 0; x < 2; x++) {
    ShImage img(m_width, m_height, 3);
  
    setupView(2, x, y);
    render();
    
    glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_FLOAT, img.data());

    const float* id = img.data();
    for (int b = 0; b < m_height; b++) for (int a = 0; a < m_width; a++) {
      int row = (m_height * 2 - 1) - (m_height * y + b);
      int col = m_width * x + a;
      for (int i = 0; i < 3; i++) {
        fd[(row * m_width * 2 + col)*3 + i ] = id[(b * m_width + a)*3 + i];
      }
    }
//     std::ostringstream s;
//     s << "_" << x << "_" << y;
//     img.savePng(filename + s.str());
  }
  final.savePng(filename);

  setupView();
  render();
}

void ShrikeCanvas::init()
{
  if (m_init) return;

  glEnable(GL_DEPTH_TEST);

  glClearColor(m_bg_r, m_bg_g, m_bg_b, 1.0);
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

void ShrikeCanvas::setBackground(unsigned char r, unsigned char g, unsigned char b)
{
  m_bg_r = (float)r/255.0;
  m_bg_g = (float)g/255.0;
  m_bg_b = (float)b/255.0;

  glClearColor(m_bg_r, m_bg_g, m_bg_b, 1.0);
  
  SetCurrent();
  render();
}

void ShrikeCanvas::keyDown(wxKeyEvent& evt)
{
  ShrikeFrame::instance()->keyDown(evt);
}
