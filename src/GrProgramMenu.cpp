#include "GrProgramMenu.hpp"
#include <string>
#include <vector>
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
              const std::string& label = program->name())
  {
    m_programs.push_back(program);
    Append(m_programs.size() - 1, label.c_str());
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

wxMenu* makeProgramMenu(GrView* view, int evx, int evy)
{
  wxMenu* menu = new wxMenu();

  wxMenu* arithmetic = new wxMenu();

  ProgramMenu* arithmetic = new ProgramMenu(view, evx, evy);

  ProgramMenu* addm = new ProgramMenu(view, evx, evy);
  addm->append(add<ShAttrib1f>(), "1");
  addm->append(add<ShAttrib1f>(), "2");
  addm->append(add<ShAttrib1f>(), "3");
  addm->append(add<ShAttrib1f>(), "4");
  arithmetic->Append(0, "add", addm);

  arithmetic->append(sub<ShAttrib4f>());
  arithmetic->append(mul<ShAttrib4f>());
  arithmetic->append(div<ShAttrib4f>());
  arithmetic->append(lerp<ShAttrib4f, ShAttrib1f>());
  // TODO arithmetic->append(mad<ShAttrib4f>());
  arithmetic->AppendSeparator();
  arithmetic->append(dot<ShAttrib3f>());
  arithmetic->AppendSeparator();
  arithmetic->append(min<ShAttrib4f>());
  arithmetic->append(max<ShAttrib4f>());

  menu->Append(0, "Arithmetic", arithmetic);

  ProgramMenu* math = new ProgramMenu(view, evx, evy);

  math->append(abs<ShAttrib4f>());
  math->append(frac<ShAttrib4f>());
  math->append(fmod<ShAttrib4f>());
  math->append(sin<ShAttrib4f>());
  math->append(sqrt<ShAttrib4f>());
  math->append(normalize<ShAttrib4f>());
  math->append(pos<ShAttrib4f>());
    
  menu->Append(0, "Math", math);

  ProgramMenu* boolean = new ProgramMenu(view, evx, evy);

  boolean->append(slt<ShAttrib4f>());
  boolean->append(sle<ShAttrib4f>());
  boolean->append(sgt<ShAttrib4f>());
  boolean->append(sge<ShAttrib4f>());
  boolean->append(seq<ShAttrib4f, ShAttrib1f>());
  boolean->append(sne<ShAttrib4f, ShAttrib1f>());

  menu->Append(0, "Boolean", boolean);

  ProgramMenu* trig = new ProgramMenu(view, evx, evy);

  trig->append(acos<ShAttrib4f>());
  trig->append(asin<ShAttrib4f>());
  trig->append(cos<ShAttrib4f>());
  trig->append(sin<ShAttrib4f>());

  menu->Append(0, "Trigonometric", trig);

  ProgramMenu* misc = new ProgramMenu(view, evx, evy);

  misc->append(dup<ShAttrib4f>());
  misc->append(fillcast<ShAttrib1f, ShAttrib2f>());
  misc->append(fillcast<ShAttrib1f, ShAttrib3f>());
  misc->append(fillcast<ShAttrib1f, ShAttrib4f>());

  menu->Append(0, "Miscellaneous", misc);


  
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
