// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#include "UniformPanel.hpp"
#include <sh/ShProgram.hpp>
#include <iostream>
#include <cmath>
#include <utility>
#include <algorithm>
#include <wx/colordlg.h>
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
    : wxSlider(parent, -1, 0, (int)(var->lowBound()*100.0f), (int)(var->highBound()*100.0f),
               wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS), 
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

class TextureButton : public wxButton {
public:
  TextureButton(wxWindow* parent,
                const ShTextureNodePtr& node,
                const ShProgram& program)
    : wxButton(parent, -1, "Set texture"),
      m_node(node),
      m_program(program)
  {
  }

  void clicked(wxCommandEvent& event)
  {
    wxFileDialog* dialog = new wxFileDialog(this, "Open Texture",
                                            SHMEDIA_DIR "/textures", "",
                                            "PNG Images (*.png)|*.png", wxOPEN);

    if (dialog->ShowModal() == wxID_OK) {
      ShImage img;
      img.loadPng(dialog->GetPath().c_str());
      m_node->memory(img.memory());
      if (m_node->dims() == SH_TEXTURE_1D) {
        m_node->setTexSize(img.width() * img.height());
      } else {
        m_node->setTexSize(img.width(), img.height());
      }
      // TODO: Replace with shUpdate when that's done.
      shBind(m_program);
      ShrikeCanvas::instance()->render();
    }
  }
  
private:

  ShTextureNodePtr m_node;
  ShProgram m_program;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(TextureButton, wxButton)
  EVT_BUTTON(-1, TextureButton::clicked)
END_EVENT_TABLE()

class ColorButton : public wxButton {
public:
  ColorButton(wxWindow* parent,
              const ShVariableNodePtr& node)
    : wxButton(parent, -1, "Set colour"),
      m_node(node)
  {
  }

  void clicked(wxCommandEvent& event)
  {

    wxColourData data;

    
    wxColour old(static_cast<unsigned char>(m_node->getValue(0) * 255.0),
                 static_cast<unsigned char>(m_node->getValue(1) * 255.0),
                 static_cast<unsigned char>(m_node->getValue(2) * 255.0));
    
    data.SetColour(old);
    
    wxColourDialog dialog(this, &data);
    if (dialog.ShowModal() == wxID_OK) {
      wxColourData& cd = dialog.GetColourData();
      wxColour& c = cd.GetColour();
      m_node->setValue(0, (float)c.Red()/255.0);
      m_node->setValue(1, (float)c.Green()/255.0);
      m_node->setValue(2, (float)c.Blue()/255.0);
      
      ShrikeCanvas::instance()->render();
    }
  }
  
private:

  ShVariableNodePtr m_node;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ColorButton, wxButton)
  EVT_BUTTON(-1, ColorButton::clicked)
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
      for (ShProgramNode::VarList::iterator I = prg.node()->uniforms.begin(); I != prg.node()->uniforms.end(); ++I) {
        ShVariableNodePtr var = *I;
        if (var->kind() != SH_TEMP) continue;
        if (var->internal()) continue;
        if (!var->has_name()) continue;
        wxSizer* lps = new wxBoxSizer(wxVERTICAL);
        wxPanel* lp = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 
                                  wxSUNKEN_BORDER);
        AnimCheckBox* cb = new AnimCheckBox(lp, var);
        lps->Add(cb, 0);
        wxStaticText* label = new wxStaticText(lp, -1, var->name().c_str());
        lps->Add(label, 0, wxALIGN_CENTER);
        lp->SetSizerAndFit(lps);
        sizer->Add(lp, 0, wxEXPAND);

        if (var->specialType() == SH_COLOR
            && var->lowBound() == 0.0
            && var->highBound() == 1.0) {
          ColorButton* button = new ColorButton(this, var);
          sizer->Add(button);
        } else {
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
      for (ShProgramNode::TexList::iterator I = prg.node()->textures.begin(); I != prg.node()->textures.end(); ++I) {
        ShTextureNodePtr tex = *I;
        if (tex->dims() == SH_TEXTURE_3D || tex->dims() == SH_TEXTURE_CUBE) continue;

        if (tex->internal()) continue;
        std::string label = tex->name();

        switch (tex->dims()) {
        case SH_TEXTURE_1D:
          label += " (1D)";
          break;
        case SH_TEXTURE_2D:
          label += " (2D)";
          break;
        case SH_TEXTURE_RECT:
          label += " (Rect)";
          break;
        case SH_TEXTURE_3D:
          label += " (3D)";
          break;
        case SH_TEXTURE_CUBE:
          label += " (Cube)";
          break;
        }
        
        wxStaticText* t = new wxStaticText(this, -1, label.c_str());
        sizer->Add(t, 0, wxALIGN_CENTER);

        wxButton* button = new TextureButton(this, tex, prg);
        
        sizer->Add(button);
      }
    }
  }
  sizer->Layout();
  FitInside();
  SetScrollRate(0, 20);
}
