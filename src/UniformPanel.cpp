#include "UniformPanel.hpp"
#include <sh/sh.hpp>
#include <iostream>
#include "ShrikeCanvas.hpp"
using namespace SH;

UniformPanel::UniformPanel(wxWindow* parent)
  : wxScrolledWindow(parent, -1),
    m_sizer(0)
{
  Show();
}

class AttribSlider : public wxSlider {
public:
  AttribSlider(wxWindow* parent, ShVariableNodePtr var, int i)
    : wxSlider(parent, -1, 0, (int)(var->lowBound()*100.0f), (int)(var->highBound()*100.0f)),
      m_var(var), m_index(i)
  {
    SetValue((int)(var->getValue(i)*100.0f));
  }

  void scroll(wxScrollEvent& event)
  {
    m_var->setValue(m_index, (float)event.GetPosition()/100.0f);
    ShrikeCanvas::instance()->render();
  }

private:
  ShVariableNodePtr m_var;
  int m_index;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AttribSlider, wxSlider)
  EVT_SCROLL(AttribSlider::scroll)
END_EVENT_TABLE()

void UniformPanel::setShader(Shader* shader)
{
  DestroyChildren();

  wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 2, 2);
  sizer->AddGrowableCol(1);
  SetSizer(sizer);
  
  if (shader) {
    int p = 0;
    for (ShProgram prg = shader->vertex(); p < 2; prg = shader->fragment(), p++) {
      for (ShProgramNode::VarList::iterator I = prg->uniforms.begin(); I != prg->uniforms.end();
           I++) {
        ShVariableNodePtr var = *I;
        if (var->internal()) continue;
        wxSizer* lps = new wxBoxSizer(wxVERTICAL);
        wxPanel* lp = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 
                                  wxSUNKEN_BORDER);
        wxStaticText* label = new wxStaticText(lp, -1, var->name().c_str());
        lps->Add(label, 0, wxALIGN_CENTER);
        lp->SetSizerAndFit(lps);
        sizer->Add(lp, 0, wxEXPAND);
        wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
        for (int i = 0; i < var->size(); i++) {
          AttribSlider* slider = new AttribSlider(this, var, i);
          vsizer->Add(slider, 0, wxEXPAND);
        }
        sizer->Add(vsizer, 1, wxEXPAND);
      }
    }
  }
  sizer->Layout();
  FitInside();
  SetScrollRate(0, 20);
}
