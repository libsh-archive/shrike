#include "UniformPanel.hpp"
#include <sh/sh.hpp>
#include <iostream>
#include <cmath>
#include <utility>
#include <algorithm>
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
    setFloatValue(var->getValue(i));
  }

  void setFloatValue(float value)
  {
    SetValue((int)(value*100.0f));
  }
  
  void scroll(wxScrollEvent& event)
  {
    m_var->setValue(m_index, (float)event.GetPosition()/100.0f);
    ShrikeCanvas::instance()->render();
  }

  void next(AttribSlider* n) { m_next = n; }
  AttribSlider* next() const { return m_next; }
  
private:
  ShVariableNodePtr m_var;
  int m_index;
  AttribSlider* m_next;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AttribSlider, wxSlider)
  EVT_SCROLL(AttribSlider::scroll)
END_EVENT_TABLE()

class UniformTimer : public wxTimer {
public:
  
  static UniformTimer* instance();

  void clear();
  void add(const ShVariableNodePtr& var, float step,
           AttribSlider* slider);
  void remove(const ShVariableNodePtr& var);

  void Notify();

  struct VarAnim {
    VarAnim(const ShVariableNodePtr& var, float step,
            AttribSlider* slider)
      : var(var), step(step), slider(slider)
    {
    }
    ShVariableNodePtr var;
    float step;
    AttribSlider* slider;
  };
  
private:
  static UniformTimer* m_instance;

  typedef std::list<VarAnim> VarAnimList;
  
  VarAnimList m_vars;
  
  UniformTimer();
  ~UniformTimer();

  
  // NOT IMPLEMENTED
  UniformTimer(const UniformTimer& other);
  UniformTimer& operator=(const UniformTimer& other);
};
UniformTimer* UniformTimer::m_instance = 0;

UniformTimer* UniformTimer::instance()
{
  if (!m_instance) m_instance = new UniformTimer();
  return m_instance;
}

UniformTimer::UniformTimer()
{
}

UniformTimer::~UniformTimer()
{
}

void UniformTimer::clear()
{
  m_vars.clear();
  Stop();
}

void UniformTimer::add(const ShVariableNodePtr& var,
                       float step, AttribSlider* slider)
{
  m_vars.push_back(VarAnim(var, step, slider));
  if (m_vars.size() == 1) Start(50);
}

bool uniform_var_equal(UniformTimer::VarAnim va,
                       ShVariableNodePtr var)
{
  return va.var == var;
}

void UniformTimer::remove(const ShVariableNodePtr& var)
{
  m_vars.remove_if(std::bind2nd(std::ptr_fun(uniform_var_equal), var));
  if (m_vars.empty()) Stop();
}

void UniformTimer::Notify()
{
  for (VarAnimList::iterator I = m_vars.begin(); I != m_vars.end(); ++I) {
    ShVariableNodePtr var = I->var;

    AttribSlider* slider = I->slider;
    for (int i = 0; i < var->size(); i++) {
      float val = var->getValue(i);
      float low = var->lowBound();
      float high = var->highBound();
      val = std::fmod((val + I->step) - low, high - low) + low; 
      var->setValue(i, val);
      if (slider) {
        slider->setFloatValue(val);
        slider = slider->next();
      }
    }
  }

  wxStopWatch watch;
  ShrikeCanvas::instance()->render();
  long delta = watch.Time();
  long interval = GetInterval();
  if (delta > interval) {
    Stop();
    Start(delta + 10);
  } else if (delta < interval - 50) {
    Stop();
    Start(std::min(delta, 50L));
  }
}

class AnimCheckBox : public wxCheckBox {
public:
  AnimCheckBox(wxWindow* parent,
               const ShVariableNodePtr& node)
    : wxCheckBox(parent, -1, ""),
      m_node(node),
      m_slider(0)
  {
  }

  ~AnimCheckBox()
  {
  }

  void slider(AttribSlider* s) { m_slider = s; }
  
  void check(wxCommandEvent& event)
  {
    if (event.IsChecked()) {
      UniformTimer::instance()->add(m_node,
                                    (m_node->highBound() - m_node->lowBound())/80.0f,
                                    m_slider);
    } else {
      UniformTimer::instance()->remove(m_node);
    }
  }

private:

  DECLARE_EVENT_TABLE()
    
  ShVariableNodePtr m_node;
  AttribSlider* m_slider;
};

BEGIN_EVENT_TABLE(AnimCheckBox, wxCheckBox)
  EVT_CHECKBOX(-1, AnimCheckBox::check)
END_EVENT_TABLE()

void UniformPanel::setShader(Shader* shader)
{
  DestroyChildren();

  wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 2, 2);
  sizer->AddGrowableCol(1);
  SetSizer(sizer);

  UniformTimer::instance()->clear();
  
  if (shader) {
    int p = 0;
    for (ShProgram prg = shader->vertex(); p < 2; prg = shader->fragment(), p++) {
      for (ShProgramNode::VarList::iterator I = prg->uniforms.begin(); I != prg->uniforms.end();
           I++) {
        ShVariableNodePtr var = *I;
        if (var->kind() != SH_TEMP) continue;
        if (var->internal()) continue;
        if (!var->hasName()) continue;
        wxSizer* lps = new wxBoxSizer(wxVERTICAL);
        wxPanel* lp = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 
                                  wxSUNKEN_BORDER);
        AnimCheckBox* cb = new AnimCheckBox(lp, var);
        lps->Add(cb, 0);
        wxStaticText* label = new wxStaticText(lp, -1, var->name().c_str());
        lps->Add(label, 0, wxALIGN_CENTER);
        lp->SetSizerAndFit(lps);
        sizer->Add(lp, 0, wxEXPAND);
        wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
        AttribSlider* last = 0;
        for (int i = 0; i < var->size(); i++) {
          AttribSlider* slider = new AttribSlider(this, var, i);
          if (i == 0) cb->slider(slider);
          vsizer->Add(slider, 0, wxEXPAND);
          if (last) last->next(slider);
          last = slider;
        }
        sizer->Add(vsizer, 1, wxEXPAND);
      }
    }
  }
  sizer->Layout();
  FitInside();
  SetScrollRate(0, 20);
}
