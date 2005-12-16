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
#include "UniformPanel.hpp"
#include <sh/shutil.hpp>
#include <sh/ShProgram.hpp>
#include <iostream>
#include <cmath>
#include <utility>
#include <algorithm>
#include <wx/colordlg.h>
#include <wx/event.h>
#include "ShrikeCanvas.hpp"
#include "ShrikeFrame.hpp"

// Defined on apple
#ifdef check
# undef check
#endif

using namespace SH;

UniformPanel::UniformPanel(wxWindow* parent)
  : wxScrolledWindow(parent, -1),
    m_sizer(0)
{
  Show();
}

class Slider : public wxSlider {
public:
  Slider(wxWindow* parent, ShVariableNodePtr var, int i)
    : wxSlider(parent, -1, 0, 0,1,
               wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS), 
      m_var(var), m_index(i)
  {
  }

  virtual void set_value() = 0;
  
  void next(Slider* n) { m_next = n; }
  Slider* next() const { return m_next; }
  
protected:
  ShVariableNodePtr m_var;
  int m_index;
  Slider* m_next;
};

template <typename T>
class AttribSlider : public Slider
{
public:
  AttribSlider(wxWindow* parent, ShVariableNodePtr var, int i, T scale)
    : Slider(parent, var, i), m_scale(scale)
  {
    SetRange((int)((*variant_convert<T, SH_HOST>(var->lowBoundVariant()))[i]*m_scale), 
             (int)((*variant_convert<T, SH_HOST>(var->highBoundVariant()))[i]*m_scale));
    set_value();
  }
  void set_value()
  {
    SetValue((int)((*variant_convert<T, SH_HOST>(m_var->getVariant()))[m_index]*m_scale));
  }
private:
  void on_scroll(wxScrollEvent& event)
  {
    T value = event.GetPosition()/m_scale;
    m_var->setVariant(new ShDataVariant<T, SH_HOST>(1, value),m_index);
    ShrikeCanvas::instance()->render();
  }
  T m_scale;
  DECLARE_EVENT_TABLE()
};

typedef AttribSlider<int> IntSlider;
typedef AttribSlider<float> FloatSlider;

#define BEGIN_TEMPLATE_EVENT_TABLE(theClass, baseClass) \
template<> \
const wxEventTable *theClass::GetEventTable() const \
{ return &theClass::sm_eventTable; } \
template<> \
const wxEventTable theClass::sm_eventTable = \
{ &baseClass::sm_eventTable, &theClass::sm_eventTableEntries[0] }; \
template<> \
wxEventHashTable theClass::sm_eventHashTable(theClass::sm_eventTable); \
template<> \
wxEventHashTable &theClass::GetEventHashTable() const \
{ return theClass::sm_eventHashTable; } \
template<> \
const wxEventTableEntry theClass::sm_eventTableEntries[] = { \

BEGIN_TEMPLATE_EVENT_TABLE(IntSlider, Slider)
  EVT_SCROLL(IntSlider::on_scroll)
END_EVENT_TABLE()
BEGIN_TEMPLATE_EVENT_TABLE(FloatSlider, Slider)
  EVT_SCROLL(FloatSlider::on_scroll)
END_EVENT_TABLE()

class UniformTimer : public wxTimer {
public:
  
  static UniformTimer* instance();

  void clear();
  void add(const ShVariableNodePtr& var, float step,
           Slider* slider);
  void remove(const ShVariableNodePtr& var);

  void Notify();

  struct VarAnim {
    VarAnim(const ShVariableNodePtr& var, float step,
            Slider* slider)
      : var(var), step(step), slider(slider)
    {
    }
    ShVariableNodePtr var;
    float step;
    Slider* slider;
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
                       float step, Slider* slider)
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

    Slider* slider = I->slider;
    for (int i = 0; i < var->size(); i++) {
      float val = (*variant_convert<float, SH_HOST>(var->getVariant()))[i];
      float low = (*variant_convert<float, SH_HOST>(var->lowBoundVariant()))[i];
      float high = (*variant_convert<float, SH_HOST>(var->highBoundVariant()))[i];
      val = std::fmod((val + I->step) - low, high - low) + low; 
      var->setVariant(new ShDataVariant<float, SH_HOST>(1, val), i);
  
      if (slider) {
        slider->set_value();
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
    : wxCheckBox(parent, -1, wxConvLibc.cMB2WX(node->name().c_str())),
      m_node(node),
      m_slider(0)
  {
  }

  ~AnimCheckBox()
  {
  }

  void slider(Slider* s) { m_slider = s; }
  
  void check(wxCommandEvent& event)
  {
    if (event.IsChecked()) {
      float high = (*variant_convert<float, SH_HOST>(m_node->highBoundVariant()))[0];
      float low = (*variant_convert<float, SH_HOST>(m_node->lowBoundVariant()))[0];
      UniformTimer::instance()->add(m_node,
                                    (high - low)/80.0f,
                                    m_slider);
    } else {
      UniformTimer::instance()->remove(m_node);
    }
  }

private:

  DECLARE_EVENT_TABLE()
    
  ShVariableNodePtr m_node;
  Slider* m_slider;
};

BEGIN_EVENT_TABLE(AnimCheckBox, wxCheckBox)
  EVT_CHECKBOX(-1, AnimCheckBox::check)
END_EVENT_TABLE()

class TextureButton : public wxBitmapButton {
public:
  TextureButton(wxWindow* parent,
                const ShTextureNodePtr& node)
    : wxBitmapButton(parent, -1, wxBitmap()),
      m_node(node)
  {
    relabel();
  }

  void clicked(wxCommandEvent& event)
  {
    wxFileDialog* dialog = new wxFileDialog(this, wxT("Open Texture"),
                                            SHMEDIA_DIR wxT("/textures"), wxT(""),
                                            wxT("PNG Images (*.png)|*.png"), wxOPEN);

    if (dialog->ShowModal() == wxID_OK) {
      ShImage img;
      // Lame convertion from wxString to std::string:
      std::string stdname;
      stdname = wxConvLibc.cWX2MB(dialog->GetPath());
      ShUtil::load_PNG(img, stdname);
      m_node->memory(img.memory(), 0);
      if (m_node->dims() == SH_TEXTURE_1D) {
        m_node->setTexSize(img.width() * img.height());
      } else {
        m_node->setTexSize(img.width(), img.height());
      }
      shUpdate();
      ShrikeCanvas::instance()->render();
      relabel();
    }
  }
  
private:
  void relabel()
  {
    unsigned char data[m_node->width()*m_node->height()*3];
    bool copied = false;

    ShHostStoragePtr storage = shref_dynamic_cast<ShHostStorage>(m_node->memory(0)->findStorage("host"));
    if (storage) {
      storage->sync();

      size_t elements = (storage->length()/(m_node->width()*m_node->height()))/storage->value_size();

      size_t count = m_node->width()*m_node->height();
      // TODO: check the storage type!
      float* tex_data = (float*)storage->data();

      copied = true;
      if (elements == 1) {
        for (size_t i = 0; i < count; ++i) {
          data[3*i+0] = (unsigned char)(tex_data[i]*255.0);
          data[3*i+1] = (unsigned char)(tex_data[i]*255.0);
          data[3*i+2] = (unsigned char)(tex_data[i]*255.0);
        }
      }
      else if (elements == 3) {
        for (size_t i = 0; i < 3*count; ++i) {
          data[i] = (unsigned char)(tex_data[i]*255.0);
        }
      }
      else 
        copied = false;
    }

    if (copied) {
      wxImage image(m_node->width(), m_node->height(), data, true);
      wxBitmap bitmap(image.Scale(64,64));
      SetBitmapLabel(bitmap);
    }
    else {
      SetBitmapLabel(wxBitmap(64,64));
    }
  }

  ShTextureNodePtr m_node;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(TextureButton, wxBitmapButton)
  EVT_BUTTON(-1, TextureButton::clicked)
END_EVENT_TABLE()

class DepButton : public wxButton {
public:
  DepButton(wxWindow* parent, const ShVariableNodePtr& node)
    : wxButton(parent, -1, wxConvLibc.cMB2WX(node->name().c_str())),
      m_node(node)
  {
  }

  void clicked(wxCommandEvent& event)
  {
    if (!m_node->evaluator()) return;

    ShProgram prg(m_node->evaluator());
    ShrikeFrame::instance()->show_ir(prg, wxConvLibc.cMB2WX(m_node->name().c_str()) );
  }
  
private:

  ShVariableNodePtr m_node;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DepButton, wxButton)
  EVT_BUTTON(-1, DepButton::clicked)
END_EVENT_TABLE()

class ColorButton : public wxBitmapButton {
public:
  ColorButton(wxWindow* parent,
              const ShVariableNodePtr& node)
    : wxBitmapButton(parent, -1, wxBitmap()),
      m_node(node)
  {
    wxColour c;
    get_node_colour(c);
    set_colour(c);
  }

  void get_node_colour(wxColour &c)
  {
    ShPointer<ShDataVariant<float, SH_HOST> > variant_ptr = 
      variant_convert<float, SH_HOST>(m_node->getVariant());
    const ShDataVariant<float, SH_HOST> &variant = *variant_ptr;
    c.Set(static_cast<unsigned char>(variant[0] * 255.0),
          static_cast<unsigned char>(variant[1] * 255.0),
          static_cast<unsigned char>(variant[2] * 255.0));
  }
  
  void clicked(wxCommandEvent& event)
  {
    wxColourData data;

    wxColour old;
    get_node_colour(old);
    
    data.SetColour(old);
    
    wxColourDialog dialog(this, &data);
    if (dialog.ShowModal() == wxID_OK) {
      wxColourData& cd = dialog.GetColourData();
      wxColour& c = cd.GetColour();

      m_node->setVariant(new ShDataVariant<float, SH_HOST>(1, (float)c.Red()/255.0f), 0);
      m_node->setVariant(new ShDataVariant<float, SH_HOST>(1, (float)c.Green()/255.0f), 1);
      m_node->setVariant(new ShDataVariant<float, SH_HOST>(1, (float)c.Blue()/255.0f), 2);
      set_colour(c);
      
      ShrikeCanvas::instance()->render();
    }
  }
  
private:
  void set_colour(const wxColour& c)
  {
    wxBitmap bitmap(24,24); 
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
    wxBrush brush(c);
    dc.SetBrush(brush);
    dc.BeginDrawing();
    dc.DrawRectangle(0,0,24,24);
    dc.EndDrawing();
    SetBitmapLabel(bitmap);
  }

  ShVariableNodePtr m_node;
  
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ColorButton, wxButton)
  EVT_BUTTON(-1, ColorButton::clicked)
END_EVENT_TABLE()

class CollapsePanel : public wxPanel
{
public:
  CollapsePanel(wxWindow* parent, const wxString &label)
    : wxPanel(parent, -1), m_collapsed(false)
  {
    m_vsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_vsizer);

    m_button = new wxButton(this, -1, label, wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    wxFont font = m_button->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_button->SetFont(font);
    m_vsizer->Add(m_button, 0, wxEXPAND);
   
    m_panel = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
    m_vsizer->Add(m_panel, 1, wxEXPAND);

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_sizer, 1, wxEXPAND|wxALL, 2);
    m_panel->SetSizer(sizer);

    collapse();
  }

  wxWindow* window() { return m_panel; }
  wxSizer* sizer() { return m_sizer; }

private:
  void collapse()
  {
    m_vsizer->Show(1, m_collapsed);
    m_parent->Layout();
  }
  void on_button(wxCommandEvent& event)
  {
    m_collapsed = !m_collapsed;
    collapse();
  }

  bool m_collapsed;
  wxButton* m_button;
  wxBoxSizer* m_vsizer;
  wxSizer* m_sizer;
  wxPanel* m_panel;

  DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(CollapsePanel, wxPanel)
  EVT_BUTTON(-1, CollapsePanel::on_button)
END_EVENT_TABLE()

void add_var(const ShVariableNodePtr& var, CollapsePanel* panel, CollapsePanel* anim)
{
  AnimCheckBox* cb = 0;
  if (!var->evaluator()) {
    cb = new AnimCheckBox(anim->window(), var);
    anim->sizer()->Add(cb, 0);
  }

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  Slider* last = 0;
  for (int i = 0; i < var->size(); i++) {
    Slider* slider = 0;
    if (shIsInteger(var->valueType())) {
      slider = new AttribSlider<int>(panel->window(), var, i, 1);
    }
    else if (shIsFloat(var->valueType())) {
      slider = new AttribSlider<float>(panel->window(), var, i, 100.0);
    }
    else {
      continue;
    }
    if (i == 0) cb->slider(slider);
    vsizer->Add(slider, 0, wxEXPAND);
    if (last) last->next(slider);
    last = slider;
  }
  panel->sizer()->Add(vsizer, 0, wxEXPAND);
}
  
void add_dep(const ShVariableNodePtr& var, CollapsePanel* panel)
{
  DepButton* button = new DepButton(panel->window(), var);
  panel->sizer()->Add(button, 0, wxEXPAND | wxALL, 2);
}

void add_color(const ShVariableNodePtr& var, CollapsePanel* panel, CollapsePanel* anim)
{
  AnimCheckBox* cb = 0;
  if (!var->evaluator()) {
    cb = new AnimCheckBox(anim->window(), var);
    anim->sizer()->Add(cb, 0);
  }

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText* text = new wxStaticText(panel->window(), -1, wxConvLibc.cMB2WX(var->name().c_str()));
  ColorButton* button = new ColorButton(panel->window(), var);
  hsizer->Add(button, 0, wxLEFT, 3);
  hsizer->Add(text, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 3);
  panel->sizer()->Add(hsizer, 0, wxBOTTOM, 3);
}

void add_palette(const ShPaletteNodePtr& pal, CollapsePanel* panel)
{
  if (pal->has_name()) 
    std::cout << pal->name() << std::endl;

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  panel->sizer()->Add(hsizer);
  for (unsigned i = 0; i < pal->palette_length(); i++) {
    ColorButton* button = new ColorButton(panel->window(), pal->get_node(i));
    hsizer->Add(button,0, wxLEFT, 4);
  }
}

void add_texture(const ShTextureNodePtr& tex, CollapsePanel* panel)
{
  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  panel->sizer()->Add(hsizer);

  hsizer->Add(new TextureButton(panel->window(), tex), 0, wxALL, 2);
  wxString name(wxConvLibc.cMB2WX(tex->name().c_str()));
  hsizer->Add(new wxStaticText(panel->window(), -1, name), 1, wxALIGN_CENTER_VERTICAL);
}

void UniformPanel::setShader(Shader* shader)
{
  DestroyChildren();
  m_vars.clear();

  UniformTimer::instance()->clear();

  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* attrib_sizer = new wxBoxSizer(wxVERTICAL);

  const int spacing=5;

  CollapsePanel *col_panel = 0;
  CollapsePanel *tex_panel = 0;
  CollapsePanel *pal_panel = 0;
  CollapsePanel *dep_panel = 0;
  CollapsePanel *anim_panel = 0;

  if (shader) {
    int p = 0;
    for (ShProgram prg = shader->vertex(); p < 2; prg = shader->fragment(), p++) {

      for (ShProgramNode::VarList::const_iterator I = prg.begin_all_parameters(); 
           I != prg.end_all_parameters(); ++I) {

        const ShVariableNodePtr& var = *I;

        if (var->kind() != SH_TEMP) continue;
        if (var->internal()) continue;
        if (!var->has_name()) continue;

        if (std::find(m_vars.begin(), m_vars.end(), var) != m_vars.end()) continue;
        m_vars.push_back(var);

        if (var->evaluator()) { 
          if (!dep_panel) dep_panel = new CollapsePanel(this, wxT("Dependent Uniforms"));
          add_dep(var, dep_panel);
        }
        else if (var->specialType() == SH_COLOR
          && (*variant_convert<float, SH_HOST>(var->lowBoundVariant()))[0] == 0.0
          && (*variant_convert<float, SH_HOST>(var->highBoundVariant()))[0] == 1.0) {

          if (!col_panel) col_panel = new CollapsePanel(this, wxT("Colors"));
          if (!anim_panel) anim_panel = new CollapsePanel(this, wxT("Animation")); 
          add_color(var, col_panel, anim_panel);
        }
        else {
          if (!anim_panel) anim_panel = new CollapsePanel(this, wxT("Animation")); 
          CollapsePanel *panel = new CollapsePanel(this, wxConvLibc.cMB2WX(var->name().c_str()));
          attrib_sizer->Add(panel, 0, wxEXPAND|wxBOTTOM, spacing);
          add_var(var, panel, anim_panel);
        }
      }
      for (ShProgramNode::PaletteList::const_iterator I = prg.begin_palettes(); I != prg.end_palettes(); ++I) {
        if (!pal_panel) pal_panel = new CollapsePanel(this, wxT("Palettes"));
        add_palette(*I, pal_panel);
      }
      for (ShProgramNode::TexList::const_iterator I = prg.begin_textures(); I != prg.end_textures(); ++I) {
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

        if (!tex_panel) tex_panel = new CollapsePanel(this, wxT("Textures"));
        add_texture(*I, tex_panel);
      }
    }
  }

  sizer->Add(attrib_sizer, 0, wxEXPAND);
  if (col_panel) sizer->Add(col_panel, 0, wxEXPAND|wxBOTTOM, spacing);
  if (tex_panel) sizer->Add(tex_panel, 0, wxEXPAND|wxBOTTOM, spacing);
  if (pal_panel) sizer->Add(pal_panel, 0, wxEXPAND|wxBOTTOM, spacing);
  if (dep_panel) sizer->Add(dep_panel, 0, wxEXPAND|wxBOTTOM, spacing);
  if (anim_panel) sizer->Add(anim_panel, 0, wxEXPAND);

  SetSizer(sizer);
  
  sizer->Layout();
  FitInside();
  SetScrollRate(0, 20);
}
