#include "GrView.hpp"

#include <vector>
#include <iterator>

#include <GL/gl.h>
#include <GL/glu.h>
#include <fontconfig/fontconfig.h>
#include <wx/wx.h>
#include <sh/sh.hpp>

#define OGLFT_NO_SOLID
#define OGLFT_NO_QT
#include <oglft/OGLFT.h>

#include "GrNode.hpp"
#include "GrPort.hpp"
#include "GrGen.hpp"
#include "GrProgramMenu.hpp"

#include "ShrikeFrame.hpp"

using namespace SH;

namespace {

void unproject(int xi, int yi, double& x, double& y, double& z)
{
  GLdouble mv[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mv);
  GLdouble proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);
    
  gluUnProject(xi, yi, 0, mv, proj, view, &x, &y, &z);
}

}

BEGIN_EVENT_TABLE(GrView, wxGLCanvas)
  EVT_PAINT(GrView::paint)
  EVT_SIZE(GrView::reshape)
  EVT_KEY_DOWN(GrView::keydown)
  EVT_MOTION(GrView::motion)
  EVT_LEFT_DOWN(GrView::mdown)
  EVT_LEFT_UP(GrView::mup)
  EVT_RIGHT_DOWN(GrView::mdown)
  EVT_RIGHT_UP(GrView::mup)
  EVT_MIDDLE_DOWN(GrView::mdown)
  EVT_MOUSEWHEEL(GrView::mousewheel)
END_EVENT_TABLE()
  
GrView::GrView(wxWindow* parent)
  : wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize),
    m_init(false),
    m_zoom(1.0),
    tx(0.0), ty(0.0),
    m_font(0),
    m_selected(0),
    m_connecting(0)
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

//   std::cerr << "Font filename: " << filename << std::endl;

  m_font = new OGLFT::TranslucentTexture((char*)filename, point, 100);

  if (!m_font->isValid()) {
    std::cerr << "Font invalid!" << std::endl;
  }

  m_font->setAdvance(false);
  m_font->setForegroundColor( 0., 0.0, 0.0, 1.0);

//   std::cerr << "Font advance: " << m_font->advance() << std::endl;
  
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

  for (NodeList::iterator I = m_nodes.begin(); I != m_nodes.end(); ++I) {
    (*I)->draw_box();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    (*I)->draw_labels();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
  }
  for (NodeList::iterator I = m_nodes.begin(); I != m_nodes.end(); ++I) {
    (*I)->draw_edges();
  }

  if (m_connecting) {
    GrPort::draw_edge(m_current_edge.x_from, m_current_edge.y_from, m_current_edge.x_to, m_current_edge.y_to);
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

  if (event.LeftIsDown() && m_selected) {
    double x, y, z;
    unproject(cur_x, m_height - cur_y, x, y, z);
    m_selected->moveTo(x + m_sel_dx, y + m_sel_dy);
    changed = true;
  }
  if (event.LeftIsDown() && m_connecting) {
    double x, y, z;
    unproject(cur_x, m_height - cur_y, x, y, z);
    if (m_connecting->in()) {
      m_current_edge.x_from = x;
      m_current_edge.y_from = y;
      m_current_edge.x_to = m_connecting->global_x();
      m_current_edge.y_to = m_connecting->global_y();
    } else {
      m_current_edge.x_from = m_connecting->global_x();
      m_current_edge.y_from = m_connecting->global_y();
      m_current_edge.x_to = x;
      m_current_edge.y_to = y;
    }
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

GrNode* GrView::addProgram(const ShProgram& program, int xi, int yi)
{
  double x, y, z;
  unproject(xi, m_height - yi, x,y,z);

  GrNode* node = new GrNode(program, x, y, m_font, this);
  m_nodes.push_back(node);
  paint();
  return node;
}

int GrView::addPicker(PickType type, void* data)
{
  int id = m_pickables.size();
  PickInfo info;
  info.id = id;
  info.type = type;
  info.data = data;
  m_pickables.push_back(info);

  return id;
}

int GrView::pick(int ev_x, int ev_y)
{
  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);

  SetCurrent();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPickMatrix(ev_x, m_height - ev_y, 4.0, 4.0, view);
  glOrtho(-m_width/2, m_width/2, -m_height/2, m_height/2, -1, 1);
    
  glMatrixMode(GL_MODELVIEW);

  struct GlHit {
    GLuint num_names;
    GLuint min_depth;
    GLuint max_depth;
    GLuint names[0];
  } __attribute__((packed));
    
  const int bufsize = 256;
  GLuint buffer[bufsize];

  glSelectBuffer(bufsize, buffer);
    
  std::reverse_iterator<NodeList::iterator> I(m_nodes.end());
  std::reverse_iterator<NodeList::iterator> last(m_nodes.begin());
  glRenderMode(GL_SELECT);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glInitNames();
    
  for (; I != last; ++I) {
    (*I)->draw_box();
    (*I)->draw_edges();
  }
  int hits = glRenderMode(GL_RENDER);
  std::cerr << "hits = " << hits << std::endl;

  GLuint* p = buffer;
  bool first = true;
  GLuint min_depth = 0;
  int min_name = -1;
  for (int i = 0; i < hits; i++) {
    GlHit* hit = (GlHit*)p;

//       std::cerr << "Hit: " << std::endl
//       << "  depth = " << hit->min_depth << std::endl
//       << "  #name = " << hit->num_names << std::endl
//       << "  name  = " << hit->names[hit->num_names - 1] << std::endl
//       << "  type  = ";
//       switch (m_pickables[hit->names[hit->num_names - 1]].type) {
//       case PICK_NODE:
//       std::cerr << "node"; break;
//       case PICK_PORT:
//       std::cerr << "port"; break;
//       case PICK_EDGE:
//       std::cerr << "edge"; break;
//       }
//       std::cerr << std::endl;
      
    if (first || hit->min_depth < min_depth) {
      min_depth = hit->min_depth;
      min_name = hit->names[hit->num_names - 1];
    }
    (char*)p += sizeof(GlHit) + sizeof(GLuint) * hit->num_names;
    first = false;
  }
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  return min_name;
    
}


class VarMenu : public wxMenu {
public:
  VarMenu(GrNode* node, ShSemanticType type,
          const std::string& title = "", int style = 0)
    : wxMenu(title.c_str(), style),
      m_node(node),
      m_type(type)
  {
    for (int i = 1; i <= 4; i++) {
      std::ostringstream os;
      os << "Sh" << ShSemanticTypeName[m_type] << i << "f";
      std::string s = os.str();
      Append(i, s.c_str());
    }
  }

  void select(wxCommandEvent& event)
  {
    int i = event.GetId();
    if (i < 1) return;

    ShProgram prg = SH_BEGIN_PROGRAM() {
      ShVariable var(new ShVariableNode(SH_INOUT, i, m_type));
    } SH_END;
    m_node->combine(prg);
  }
  
private:

  GrNode* m_node;
  ShSemanticType m_type;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(VarMenu, wxMenu)
  EVT_MENU(-1, VarMenu::select)
END_EVENT_TABLE()


void GrView::mdown(wxMouseEvent& event)
{
  bool changed = false;
  if (event.LeftDown()) {
    int id = pick(event.GetX(), event.GetY());
    if (id >= 0) {
      double x, y, z;
      unproject(event.GetX(), m_height - event.GetY(), x, y, z);
      if (m_pickables[id].type == PICK_NODE) {
        m_selected = reinterpret_cast<GrNode*>(m_pickables[id].data);
        
        m_sel_dx = m_selected->x() - x;
        m_sel_dy = m_selected->y() - y;
      } else if (m_pickables[id].type == PICK_PORT) {
        m_connecting = reinterpret_cast<GrPort*>(m_pickables[id].data);
        m_current_edge.x_from = x;
        m_current_edge.y_from = y;
        m_current_edge.x_to = x;
        m_current_edge.y_to = y;
        changed = true;
      }
    }
  }

  if (event.RightDown()) {
    int id = pick(event.GetX(), event.GetY());
    if (id < 0) {
      wxMenu* menu = makeProgramMenu(this, event.GetX(), event.GetY());
      PopupMenu(menu, event.GetX(), event.GetY());
    }
    if (id >= 0 && m_pickables[id].type == PICK_NODE) {
      GrNode* node = reinterpret_cast<GrNode*>(m_pickables[id].data);
      if (node == m_inputs_vtx || node == m_inbetween || node == m_outputs_frg) {
        wxMenu* menu = new wxMenu();

        {
          VarMenu* var_menu = new VarMenu(node, SH_ATTRIB);
          menu->Append(0, ShSemanticTypeName[SH_ATTRIB], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_POINT);
          menu->Append(0, ShSemanticTypeName[SH_POINT], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_VECTOR);
          menu->Append(0, ShSemanticTypeName[SH_VECTOR], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_NORMAL);
          menu->Append(0, ShSemanticTypeName[SH_NORMAL], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_TEXCOORD);
          menu->Append(0, ShSemanticTypeName[SH_TEXCOORD], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_COLOR);
          menu->Append(0, ShSemanticTypeName[SH_COLOR], var_menu);
        }
        {
          VarMenu* var_menu = new VarMenu(node, SH_POSITION);
          menu->Append(0, ShSemanticTypeName[SH_POSITION], var_menu);
        }
        PopupMenu(menu, event.GetX(), event.GetY());
      }
    } else if (id >= 0 && m_pickables[id].type == PICK_PORT) {
      GrPort* port = reinterpret_cast<GrPort*>(m_pickables[id].data);
      PopupMenu(port->contextMenu(), event.GetX(), event.GetY());
      changed = true;
    } else if (id >= 0 && m_pickables[id].type == PICK_EDGE) {
      GrEdge* edge = reinterpret_cast<GrEdge*>(m_pickables[id].data);
      unjoin(edge->from, edge->to);
      changed = true;
    }
  }
  
  if (changed) {
    paint();
  }
}

void GrView::mup(wxMouseEvent& event)
{
  bool changed = false;
  if (event.LeftUp()) {
    m_selected = 0;

    if (m_connecting) {
      int id = pick(event.GetX(), event.GetY());
      if (id >= 0 && m_pickables[id].type == PICK_PORT) {
        GrPort* other = reinterpret_cast<GrPort*>(m_pickables[id].data);
        if (m_connecting->in() == other->in()) {
          std::cerr << "Connection must be in->out or out->in" << std::endl;
        } else {
          GrPort* source = m_connecting->in() ? other : m_connecting;
          GrPort* dest = m_connecting->in() ? m_connecting : other;
          join(source, dest);
        }
      } else {
        std::cerr << "Not connecting anywhere" << std::endl;
      }
      m_connecting = 0;
      changed = true;
    }
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

  
  ShProgram prg_in_vtx = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInOutPosition4f SH_DECL(pos);
  } SH_END;
  prg_in_vtx->name("Inputs");
  ShProgram prg_inbetween = SH_BEGIN_PROGRAM("") {
    ShInOutPosition4f SH_DECL(pos);
  } SH_END;
  prg_inbetween->name("Interpolants");
  ShProgram prg_out_frg = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutColor3f SH_DECL(color);
  } SH_END;
  prg_out_frg->name("Outputs");

  m_inputs_vtx = addProgram(prg_in_vtx, 0, m_height/2);
  m_inbetween = addProgram(prg_inbetween, m_width/2, m_height/2);
  m_outputs_frg = addProgram(prg_out_frg, m_width, m_height/2);

}

class SimpleShader : public Shader {
public:
  SimpleShader(const ShProgram& vsh, const ShProgram& fsh)
    : Shader("Constructed Shader"), m_vsh(vsh), m_fsh(fsh)
  {
  }

  bool init() { return true; }

  ShProgram vertex() { return m_vsh; }
  ShProgram fragment() { return m_fsh; }

private:
  ShProgram m_vsh;
  ShProgram m_fsh;
};

void GrView::keydown(wxKeyEvent& event)
{
//   std::cerr << "Key down: " << event.GetKeyCode() << std::endl;
  if (event.GetKeyCode() == 'G') {
//     std::cerr << "Generating program" << std::endl;
    ShProgram vsh = generateShader(m_inputs_vtx, m_inbetween);
    if (!vsh) return;
    ShProgram fsh = generateShader(m_inbetween, m_outputs_frg);
    if (!fsh) return;

    vsh->code()->print(std::cerr);
    fsh->code()->print(std::cerr);

    SimpleShader* s = new SimpleShader(vsh, fsh);
    ShrikeFrame::instance()->setShader(s);
  }
}
