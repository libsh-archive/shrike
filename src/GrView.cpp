#include "GrView.hpp"

#include <vector>

#include <GL/gl.h>
#include <GL/glu.h>
#include <fontconfig/fontconfig.h>
#include <wx/wx.h>
#include <sh/sh.hpp>

#define OGLFT_NO_SOLID
#define OGLFT_NO_QT
#include <oglft/OGLFT.h>

#include "GrNode.hpp"

using namespace SH;

BEGIN_EVENT_TABLE(GrView, wxGLCanvas)
  EVT_PAINT(GrView::paint)
  EVT_SIZE(GrView::reshape)
  EVT_MOTION(GrView::motion)
  EVT_LEFT_DOWN(GrView::mdown)
  EVT_RIGHT_DOWN(GrView::mdown)
  EVT_MIDDLE_DOWN(GrView::mdown)
  EVT_MOUSEWHEEL(GrView::mousewheel)
END_EVENT_TABLE()

class ProgramMenu : public wxMenu {
public:
  ProgramMenu(GrView* view,
              int x, int y, // Coordinates to pass to addProgram
              const std::string& title = "", int style = 0)
    : wxMenu(title.c_str(), style),
      m_view(view),
      m_x(x), m_y(y)
  {
  }

  void select(wxCommandEvent& event)
  {
    int i = event.GetId();
    if (i < 0 || i >= m_programs.size()) return;
    m_view->addProgram(m_programs[i], m_x, m_y);
  }

  void append(const ShProgram& program)
  {
    m_programs.push_back(program);
    Append(m_programs.size() - 1, program->name().c_str());
  }
  
private:
  std::vector<ShProgram> m_programs;

  GrView* m_view;
  int m_x, m_y;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ProgramMenu, wxMenu)
  EVT_MENU(-1, ProgramMenu::select)
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

void GrView::addProgram(const ShProgram& program, int xi, int yi)
{
  GLdouble mv[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mv);
  GLdouble proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);
    
  double x, y, z;

  gluUnProject(xi, m_height - yi, 0, mv, proj, view, &x, &y, &z);

  m_nodes.push_back(new GrNode(program, x, y, m_font));
}

void GrView::mdown(wxMouseEvent& event)
{
  bool changed = false;
  if (event.LeftDown()) {
    changed = true;

    addProgram(mul<ShAttrib4f>(), event.GetX(), event.GetY());
  }

  if (event.RightDown()) {
    wxMenu* menu = new wxMenu();
    ProgramMenu* arithmetic = new ProgramMenu(this, event.GetX(), event.GetY());

    arithmetic->append(add<ShAttrib4f>());
    arithmetic->append(sub<ShAttrib4f>());
    arithmetic->append(mul<ShAttrib4f>());
    arithmetic->append(div<ShAttrib4f>());
    arithmetic->append(lerp<ShAttrib4f, ShAttrib1f>());
    arithmetic->append(sqrt<ShAttrib4f>());
    arithmetic->AppendSeparator();
    arithmetic->append(fmod<ShAttrib4f>());
    arithmetic->append(frac<ShAttrib4f>());
    arithmetic->AppendSeparator();
    arithmetic->append(min<ShAttrib4f>());
    arithmetic->append(max<ShAttrib4f>());
    arithmetic->AppendSeparator();
    
    menu->Append(0, "Arithmetic", arithmetic);

    ProgramMenu* boolean = new ProgramMenu(this, event.GetX(), event.GetY());

    boolean->append(slt<ShAttrib4f>());
    boolean->append(sle<ShAttrib4f>());
    boolean->append(sgt<ShAttrib4f>());
    boolean->append(sge<ShAttrib4f>());
    boolean->append(seq<ShAttrib4f, ShAttrib1f>());
    boolean->append(sne<ShAttrib4f, ShAttrib1f>());

    menu->Append(0, "Boolean", boolean);

    ProgramMenu* trig = new ProgramMenu(this, event.GetX(), event.GetY());

    trig->append(acos<ShAttrib4f>());
    trig->append(asin<ShAttrib4f>());
    trig->append(cos<ShAttrib4f>());
    trig->append(sin<ShAttrib4f>());
    
    menu->Append(0, "Trigonometric", trig);

    PopupMenu(menu, event.GetX(), event.GetY());
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
