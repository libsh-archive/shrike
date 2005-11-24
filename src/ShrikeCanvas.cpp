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
#include <sstream>

#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include "ShrikeGl.hpp"
#include <wx/glcanvas.h>
#include "ShrikeCanvas.hpp"
#include "ShrikeFrame.hpp"
#include "Globals.hpp"
#include "ShTrackball.hpp"
#include "Timer.hpp"
#include "shaders/LCDSmall.hpp"

void shrikeGlCheckError(const char* desc, const char* file, int line) {
  GLenum errnum = glGetError();
  char* error = 0;
  switch (errnum) {
  case GL_NO_ERROR:
    return;
  case GL_INVALID_ENUM:
    error = "GL_INVALID_ENUM";
    break;
  case GL_INVALID_VALUE:
    error = "GL_INVALID_VALUE";
    break;
  case GL_INVALID_OPERATION:
    error = "GL_INVALID_OPERATION";
    break;
  case GL_STACK_OVERFLOW:
    error = "GL_STACK_OVERFLOW";
    break;
  case GL_STACK_UNDERFLOW:
    error = "GL_STACK_UNDERFLOW";
    break;
  case GL_OUT_OF_MEMORY:
    error = "GL_OUT_OF_MEMORY";
    break;
  case GL_TABLE_TOO_LARGE:
    error = "GL_TABLE_TOO_LARGE";
    break;
  default:
    error = "Unknown error!";
    break;
  }
  std::cerr << "Shrike GL ERROR on " << file << ": " <<line<<": "<< error << std::endl;
  std::cerr << "Shrike GL ERROR call: " << desc << std::endl;
}


#define SHRIKE_GL_CHECK_ERROR(op) \
  op;shrikeGlCheckError( # op, (char*) __FILE__, (int) __LINE__)

#define SHRIKE_GL_IGNORE_ERROR(op) \
  op;glGetError()

#define SHRIKE_GL_CHECK_CURRENT_ERROR \
  shrikeGlCheckError( "<state>", (char*) __FILE__, (int) __LINE__)


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
    m_showFps(false),
    m_fps_shaders(0),
    m_bg_r(0.2), m_bg_g(0.2), m_bg_b(0.2),
    m_bg(0.2, 0.2, 0.2)
{
  m_instance = this;

  GetGlobals().mv.internal(true);
  GetGlobals().mvp.internal(true);
  GetGlobals().mv_inverse.internal(true);
  GetGlobals().lightPos.internal(true);
  GetGlobals().lightDirW.internal(true);
  GetGlobals().lightLenW.internal(true);

  GetGlobals().mv.name("mv");
  GetGlobals().mvp.name("mvp");
  GetGlobals().mv_inverse.name("mv_inverse");
  GetGlobals().lightPos.name("lightPos");
  GetGlobals().lightDirW.name("lightDirW");
  GetGlobals().lightLenW.name("lightLenW");

  resetView();
}

ShrikeCanvas* ShrikeCanvas::instance()
{
  return m_instance;
}

void ShrikeCanvas::paint(wxPaintEvent& event)
{
  wxPaintDC dc(this);
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  render();
}

void ShrikeCanvas::setModel(ShObjMesh* model)
{
  if (m_model == model) return;
  delete m_model;
  m_model = model;
  m_model_dirty = true;
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  render();
}

const ShObjMesh* ShrikeCanvas::getModel() const {
  return m_model;
}


void ShrikeCanvas::setShader(Shader* shader)
{
  SetCurrent();
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  if (shader) {
    shader->bind();
  } else {
    shUnbind();
  }
  SHRIKE_GL_CHECK_CURRENT_ERROR;
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
      t.resize(GetClientSize().GetWidth(), GetClientSize().GetHeight());
      ShMatrix4x4f m;
      m = inverse(GetGlobals().mv);
      m = m | t.rotate(m_last_x, m_last_y, cur_x, cur_y);
      m = m | GetGlobals().mv;
      GetGlobals().lightDirW = m | GetGlobals().lightDirW;
    } else {
      m_camera.orbit(m_last_x, m_last_y, cur_x, cur_y, GetClientSize().GetWidth(), GetClientSize().GetHeight());
    }
  }
  if (event.MiddleIsDown()) {
    if (event.ShiftDown()) {
      GetGlobals().lightLenW += dy/10.0;
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
  if (!GetContext()) return;
  
  ShTimer start;
  if(m_showFps) {
    start = ShTimer::now();
  }

  SetCurrent();
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  init();

  SHRIKE_GL_CHECK_CURRENT_ERROR;
  
  SHRIKE_GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT));

  if (m_shader) {
    m_shader->bind();
    if (!m_shader->render(*m_model))
      renderObject();
  }

  shUnbind();

  ShPoint3f lp = GetGlobals().lightDirW * GetGlobals().lightLenW;
  float pos[3];
  lp.getValues(pos);

  SHRIKE_GL_CHECK_ERROR(glPointSize(3.0));

  glBegin(GL_POINTS); {
    glColor3f(1.0, 0.0, 1.0);
    glVertex3fv(pos);
  } SHRIKE_GL_IGNORE_ERROR(glEnd()); // On ATI we get spurious errors here

  if (m_shader) {
    SHRIKE_GL_CHECK_ERROR(glFinish());
  }
  
  if (m_showFps && m_shader) {
    ShTimer end = ShTimer::now();
    double elapsed = (end - start).value() / 1000.0; 

    m_fps = 1.0 / elapsed;
    SHRIKE_GL_CHECK_ERROR(glDisable(GL_DEPTH_TEST));
    shBind(*m_fps_shaders);
    double width = 160.0/GetClientSize().GetWidth();
    double height = 80.0/GetClientSize().GetHeight(); 
    glBegin(GL_QUADS); {
      glTexCoord2f(0.0, 0.0);
      glVertex2f(-1.0, -1.0);
      glTexCoord2f(0.0, 1.0);
      glVertex2f(-1.0, -1.0 + height);
      glTexCoord2f(1.0, 1.0);
      glVertex2f(-1.0 + width, -1.0 + height);
      glTexCoord2f(1.0, 0.0);
      glVertex2f(-1.0 + width, -1.0);
    } SHRIKE_GL_IGNORE_ERROR(glEnd());
    SHRIKE_GL_CHECK_ERROR(glEnable(GL_DEPTH_TEST));
  }
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  SwapBuffers();
  SHRIKE_GL_CHECK_CURRENT_ERROR;
}

void ShrikeCanvas::renderObject()
{
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  if (m_model_list == 0) {
    m_model_list = SHRIKE_GL_CHECK_ERROR(glGenLists(1));
  }

  if (m_model_dirty) {
    SHRIKE_GL_CHECK_ERROR(glNewList(m_model_list, GL_COMPILE_AND_EXECUTE));
    float values[4];
    glBegin(GL_TRIANGLES);
    for(ShObjMesh::FaceSet::iterator I = m_model->faces.begin();
        I != m_model->faces.end(); ++I) {
      ShObjEdge *e = (*I)->edge;
      do {
        e->normal.getValues(values); 
        glNormal3fv(values);

        e->texcoord.getValues(values);
#ifdef GL_ARB_multitexture
        glMultiTexCoord2fvARB(GL_TEXTURE0, values);
#else
        glMultiTexCoord2fv(GL_TEXTURE0, values);
#endif

        e->tangent.getValues(values);
#ifdef GL_ARB_multitexture
        glMultiTexCoord3fvARB(GL_TEXTURE0 + 1, values);
#else
        glMultiTexCoord3fv(GL_TEXTURE0 + 1, values);
#endif

        e->start->pos.getValues(values);
        glVertex3fv(values);
        e = e->next;
      } while(e != (*I)->edge);
    }
    SHRIKE_GL_IGNORE_ERROR(glEnd()); // Ignore spurious ATI errors
    SHRIKE_GL_CHECK_ERROR(glEndList());
    m_model_dirty = false;
  } else {
    SHRIKE_GL_CHECK_CURRENT_ERROR;
    SHRIKE_GL_IGNORE_ERROR(glCallList(m_model_list)); // On ATI...

  }
  SHRIKE_GL_CHECK_CURRENT_ERROR;
}

void ShrikeCanvas::setupView(int nsplit, int x, int y)
{
  SHRIKE_GL_CHECK_ERROR(glMatrixMode(GL_PROJECTION));
  SHRIKE_GL_CHECK_ERROR(glLoadIdentity());

  ShMatrix4x4f split;
  
  if (nsplit > 1) {
    split[0][0] = nsplit;
    split[1][1] = nsplit;
    split[2][2] = 1.0;
    split[3][3] = 1.0;
    split[0][3] = (float)(nsplit - 1 - x*2);
    split[1][3] = (float)(nsplit - 1 - y*2);
    float values[16];
    for (int i = 0; i < 16; i++) split[i%4](i/4).getValues(&values[i]);
    SHRIKE_GL_CHECK_ERROR(glMultMatrixf(values));
  }
  
  m_camera.glProjection((float)GetClientSize().GetWidth()/GetClientSize().GetHeight());
  SHRIKE_GL_CHECK_CURRENT_ERROR;

  SHRIKE_GL_CHECK_ERROR(glMatrixMode(GL_MODELVIEW));
  SHRIKE_GL_CHECK_ERROR(glLoadIdentity());
  m_camera.glModelView();
  SHRIKE_GL_CHECK_CURRENT_ERROR;
  
  GetGlobals().mv = m_camera.shModelView();
  GetGlobals().mv_inverse = inverse(GetGlobals().mv);
  GetGlobals().mvp = m_camera.shModelViewProjection(split);
  GetGlobals().lightPos = GetGlobals().mv | ShPoint3f(GetGlobals().lightDirW * GetGlobals().lightLenW);
}

void ShrikeCanvas::screenshot(const wxString& filename)
{
  int mult = 4;
  ShImage final(GetClientSize().GetWidth()*mult, GetClientSize().GetHeight()*mult, 3);
  float* fd = final.data();
  std::string stdfilename;
    
  for (int y = 0; y < mult; y++) for (int x = 0; x < mult; x++) {
    ShImage img(GetClientSize().GetWidth(), GetClientSize().GetHeight(), 3);
  
    setupView(mult, x, y);
    render();
    
    glReadPixels(0, 0, GetClientSize().GetWidth(), GetClientSize().GetHeight(), GL_RGB, GL_FLOAT, img.data());

    const float* id = img.data();
    for (int b = 0; b < GetClientSize().GetHeight(); b++) for (int a = 0; a < GetClientSize().GetWidth(); a++) {
      int row = GetClientSize().GetHeight()*(mult - y) - b - 1;
      int col = GetClientSize().GetWidth() * x + a;
      for (int i = 0; i < 3; i++) {
        fd[(row * GetClientSize().GetWidth() * mult + col)*3 + i ] = id[(b * GetClientSize().GetWidth() + a)*3 + i];
      }
    }
#if 0
     std::ostringstream s;
     s << "_" << x << "_" << y;
     img.save_PNG(filename + s.str());
#endif
  }
  // Lame convertion from wxString to std::string:
  stdfilename = wxConvLibc.cWX2MB(filename);
  ShUtil::save_PNG(final, stdfilename, 0);

  setupView();
  render();
}

void ShrikeCanvas::init()
{
  if (m_init) return;

  shrikeGlInit();

  SHRIKE_GL_CHECK_ERROR(glEnable(GL_DEPTH_TEST));

  SHRIKE_GL_CHECK_ERROR(glClearColor(m_bg_r, m_bg_g, m_bg_b, 1.0));
  setupView();
  
  m_fpsVsh = SH_BEGIN_PROGRAM("gpu:vertex"); {
    ShInOutPosition4f SH_DECL(pos);
    ShInOutTexCoord2f SH_DECL(u);
  } SH_END;

  ShConstColor3f yellow(1, 1, 0);
  m_fpsFsh = SH_BEGIN_PROGRAM("gpu:fragment"); {
    ShInputPosition4f SH_DECL(pos);
    ShInputTexCoord2f SH_DECL(u);
    ShOutputColor3f SH_DECL(result);
    ShAttrib1f indigit = lcdSmall(u, m_fps, 4, 0, false, false, 0.23, 1.0, 0.026);
    //discard(1.0f - indigit); // TODO should be using this, but it won't fit on ATI
    result = lerp(indigit, yellow, m_bg); 
  } SH_END;

  m_fps_shaders = new ShProgramSet(m_fpsVsh, m_fpsFsh);
  
  m_init = true;
  SHRIKE_GL_CHECK_CURRENT_ERROR;
}

void ShrikeCanvas::reshape(wxSizeEvent& event)
{
  wxGLCanvas::OnSize(event);

  int w, h;

  GetClientSize(&w, &h);
  GetGlobals().width = 1.0f*w;
  GetGlobals().height = 1.0f*h;
  
  if (m_init && GetContext() && w > 0 && h > 0) {
    SetCurrent();
    SHRIKE_GL_CHECK_CURRENT_ERROR;
    SHRIKE_GL_CHECK_ERROR(glViewport(0, 0, w, h));
    setupView();
  }
}

void ShrikeCanvas::resetView()
{
  m_camera = Camera();
  m_camera.move(0, 0.0, -7.0);
  if (GetContext()) {
    SetCurrent();
    SHRIKE_GL_CHECK_CURRENT_ERROR;
    setupView();
    render();
  }
}

void ShrikeCanvas::setBackground(unsigned char r, unsigned char g, unsigned char b)
{
  m_bg_r = (float)r/255.0;
  m_bg_g = (float)g/255.0;
  m_bg_b = (float)b/255.0;
  m_bg(0) = m_bg_r; 
  m_bg(1) = m_bg_g; 
  m_bg(2) = m_bg_b; 

  SHRIKE_GL_CHECK_ERROR(glClearColor(m_bg_r, m_bg_g, m_bg_b, 1.0));
  
  SetCurrent();
  render();
}

void ShrikeCanvas::setShowFps(bool fps) {
  m_showFps = fps;

  SetCurrent();
  render();
}

void ShrikeCanvas::keyDown(wxKeyEvent& evt)
{
  ShrikeFrame::instance()->keyDown(evt);
}
