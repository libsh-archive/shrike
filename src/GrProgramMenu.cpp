#include "GrProgramMenu.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <wx/wx.h>
#include <sh/sh.hpp>
#include "Shader.hpp"
#include "GrView.hpp"

using namespace SH;

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

  void append(const ShProgram& program,
              const std::string& label = "")
  {
    std::string l = label;
    m_programs.push_back(program);
    if (l.empty()) l = program->name();
    Append(m_programs.size() - 1, l.c_str());
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

#define OP4(parent, name) \
do { \
  ProgramMenu* name ## _menu = new ProgramMenu(view, evx, evy); \
  name ## _menu->append(name <ShAttrib1f>(), "1"); \
  name ## _menu->append(name <ShAttrib2f>(), "2"); \
  name ## _menu->append(name <ShAttrib3f>(), "3"); \
  name ## _menu->append(name <ShAttrib4f>(), "4"); \
  parent->Append(0, # name, name ## _menu); \
} while (0)

template<int N>
ShProgram makeSplit()
{
  ShProgram split = SH_BEGIN_PROGRAM() {
    ShAttrib<N, SH_INPUT> in;
    for (int i = 0; i < N; i++) {
      std::ostringstream os;
      os << i;
      ShOutputAttrib1f SH_NAMEDECL(out, os.str()) = in(i);
    }
  } SH_END;
  split->name("split");
  return split;
}

template<int N>
ShProgram makeJoin()
{
  ShProgram join = SH_BEGIN_PROGRAM() {
    ShAttrib<N, SH_OUTPUT> out;
    for (int i = 0; i < N; i++) {
      std::ostringstream os;
      os << i;
      ShInputAttrib1f SH_NAMEDECL(in, os.str());
      out(i) = in;
    }
  } SH_END;
  join->name("join");
  return join;
}
  
wxMenu* makeProgramMenu(GrView* view, int evx, int evy)
{
  wxMenu* menu = new wxMenu();

  ProgramMenu* arithmetic = new ProgramMenu(view, evx, evy);

  OP4(arithmetic, add);
  OP4(arithmetic, sub);
  OP4(arithmetic, mul);
  OP4(arithmetic, div);
  OP4(arithmetic, lerp);
  // TODO OP4(arithmetic, mad);
  arithmetic->AppendSeparator();
  OP4(arithmetic, sqrt);

  menu->Append(0, "Arithmetic", arithmetic);

  ProgramMenu* clamping = new ProgramMenu(view, evx, evy);
  OP4(clamping, abs);
  OP4(clamping, frac);
  OP4(clamping, fmod);
  OP4(clamping, min);
  OP4(clamping, max);
  OP4(clamping, pos);
  menu->Append(0, "Clamping", clamping);

  ProgramMenu* geom = new ProgramMenu(view, evx, evy);
  OP4(geom, dot);
  // TODO geom->append(cross<ShVector3f>());
  geom->append(normalize<ShVector3f>());
  menu->Append(0, "Geometry", geom);

  ProgramMenu* boolean = new ProgramMenu(view, evx, evy);

  OP4(boolean, slt);
  OP4(boolean, sle);
  OP4(boolean, sgt);
  OP4(boolean, sge);
  OP4(boolean, seq);
  OP4(boolean, sne);

  menu->Append(0, "Boolean", boolean);

  ProgramMenu* trig = new ProgramMenu(view, evx, evy);

  OP4(trig, acos);
  OP4(trig, asin);
  OP4(trig, cos);
  OP4(trig, sin);

  menu->Append(0, "Trigonometric", trig);

  ProgramMenu* misc = new ProgramMenu(view, evx, evy);

  OP4(misc, dup);
  ProgramMenu* fc_menu = new ProgramMenu(view, evx, evy);
  fc_menu->append(fillcast<ShAttrib1f, ShAttrib2f>(), "1->2");
  fc_menu->append(fillcast<ShAttrib1f, ShAttrib3f>(), "1->3");
  fc_menu->append(fillcast<ShAttrib1f, ShAttrib4f>(), "1->4");
  misc->Append(0, "fillcast", fc_menu);
  menu->Append(0, "Miscellaneous", misc);

  ProgramMenu* swiz = new ProgramMenu(view, evx, evy);
  ProgramMenu* split = new ProgramMenu(view, evx, evy);
  ProgramMenu* join = new ProgramMenu(view, evx, evy);

  split->append(makeSplit<2>(), "2");
  split->append(makeSplit<3>(), "3");
  split->append(makeSplit<4>(), "4");

  join->append(makeJoin<2>(), "2");
  join->append(makeJoin<3>(), "3");
  join->append(makeJoin<4>(), "4");
  
  swiz->Append(0, "split", split);
  swiz->Append(0, "join", join);
  menu->Append(0, "Swizzling", swiz);
  
  ProgramMenu* shaders = new ProgramMenu(view, evx, evy);
  for (Shader::iterator I = Shader::begin(); I != Shader::end(); I++) {
    Shader* shader = *I;
    if (!shader->firstTimeInit()) continue;
    ShProgram vsh = shader->vertex(); // TODO: Clone program
    ShProgram fsh = shader->fragment(); // TODO: Clone program
    vsh->name(shader->name() + " (vertex)");
    fsh->name(shader->name() + " (fragment)");
    shaders->append(vsh);
    shaders->append(fsh);
  }
  menu->Append(0, "Shaders", shaders);

  return menu;
}
