#include "GrView.hpp"
#define OGLFT_NO_SOLID
#define OGLFT_NO_QT
#include <oglft/OGLFT.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <fontconfig/fontconfig.h>
#include "GrNode.hpp"

#include <sh/sh.hpp>

using namespace SH;

BEGIN_EVENT_TABLE(GrView, wxGLCanvas)
  EVT_PAINT(GrView::paint)
  EVT_SIZE(GrView::reshape)
  EVT_MOTION(GrView::motion)
  EVT_LEFT_DOWN(GrView::mdown)
  EVT_MOUSEWHEEL(GrView::mousewheel)
END_EVENT_TABLE()

GrView::GrView(wxWindow* parent)
  : wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize),
    m_init(false),
    m_zoom(1.0),
    tx(0.0), ty(0.0),
    m_font(0)
{
  FcInit();

  FcPattern* sans;
  double point = 10.0;
  sans = FcPatternBuild (NULL,
                         FC_FAMILY, FcTypeString, "Arial",
                         FC_SIZE, FcTypeDouble, point,
                         NULL);
  FcPattern* matched;
  FcResult result;
  matched = FcFontMatch (0, sans, &result);

  FcChar8* filename;
  if (FcPatternGetString (matched, FC_FILE, 0, &filename) != FcResultMatch)
    ;

  int id;
  if (FcPatternGetInteger (matched, FC_INDEX, 0, &id) != FcResultMatch)
    ;

  std::cerr << "Font filename: " << filename << std::endl;

  m_font = new OGLFT::TranslucentTexture((char*)filename, point, 100);

  if (!m_font->isValid()) {
    std::cerr << "Font invalid!" << std::endl;
  }

  m_font->setAdvance(false);
  m_font->setForegroundColor( 0., 0.0, 0.0, 1.0);

  std::cerr << "Font advance: " << m_font->advance() << std::endl;
  
  FcPatternDestroy (sans);
  FcPatternDestroy (matched);
}

GrView::~GrView()
{
  delete m_font;
}

void GrView::paint()
{
  SetCurrent();
  init();
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (std::list<GrNode*>::iterator I = m_nodes.begin(); I != m_nodes.end(); ++I) {
    (*I)->draw_box();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    (*I)->draw_labels();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
  }
  
  SwapBuffers();
}

void GrView::motion(wxMouseEvent& event)
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

  bool changed = false;
  
  if (event.MiddleIsDown()) {
    tx += dx;
    ty -= dy;
    changed = true;
  }

  if (changed) {
    SetCurrent();
    setupView();
    paint();
  }
  m_last_x = cur_x;
  m_last_y = cur_y;
}

void GrView::mousewheel(wxMouseEvent& event)
{
  double rot = event.GetWheelRotation();
  double delta = 120; //event.GetWheelDelta();

  m_zoom += rot/delta * m_zoom * 0.1;
  if (m_zoom < 0.1) m_zoom = 0.1;

  SetCurrent();
  setupView();
  paint();
}

void GrView::mdown(wxMouseEvent& event)
{
  bool changed = false;
  if (event.LeftDown()) {
    changed = true;

    GLdouble mv[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    GLdouble proj[16];
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    
    double x, y, z;
    gluUnProject(event.GetX(), m_height - event.GetY(), 0, mv, proj, view, &x, &y, &z);
    
    m_nodes.push_back(new GrNode("Foo", mul<ShAttrib4f>(), x, y, m_font));
  }
  if (changed) {
    paint();
  }
}

void GrView::reshape()
{
  if (GetContext()) {
    SetCurrent();
    glViewport(0, 0, m_width, m_height);
    setupView();
  }
}

void GrView::setupView()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-m_width/2, m_width/2, -m_height/2, m_height/2, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glTranslated(-tx, -ty, 0.0);
  glScaled(m_zoom, m_zoom, 1.0);
  glTranslated(tx/m_zoom, ty/m_zoom, 0.0);
  glTranslated(tx/m_zoom, ty/m_zoom, 0.0);
}

void GrView::init()
{
  if (m_init) return;

  glClearColor(0.5, 0.5, 0.5, 1.0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glDisable(GL_LIGHTING);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  setupView();
  
  m_init = true;
}
