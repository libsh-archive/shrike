#ifndef SHRIKEFRAME_HPP
#define SHRIKEFRAME_HPP

#include <wx/wx.h>
#include "UniformPanel.hpp"

enum {
  SHRIKE_MENU_OPEN_MODEL,
  SHRIKE_MENU_QUIT,

  SHRIKE_LISTBOX_SHADERS
};

class ShrikeCanvas;

class ShrikeFrame : public wxFrame {
public:
  ShrikeFrame();
  virtual ~ShrikeFrame();

  void quit(wxCommandEvent& event);
  void openModel(wxCommandEvent& event);

  void setShader(wxCommandEvent& event);
  
private:
  wxListBox* initShaderList(wxWindow* parent);

  ShrikeCanvas* m_canvas;
  UniformPanel* m_panel;
  
  DECLARE_EVENT_TABLE()
};

#endif
