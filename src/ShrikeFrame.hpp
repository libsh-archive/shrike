#ifndef SHRIKEFRAME_HPP
#define SHRIKEFRAME_HPP

#include <wx/wx.h>
#include "UniformPanel.hpp"
#include "Shader.hpp"
#include "ShrikePropsDialog.hpp"
enum {
  SHRIKE_MENU_OPEN_MODEL,
  SHRIKE_MENU_QUIT,
  
  SHRIKE_MENU_SHADER_PROPS,
  
  SHRIKE_MENU_SHADER_SHOW_VSH,
  SHRIKE_MENU_SHADER_SHOW_FSH,

  SHRIKE_MENU_SHADER_REINIT,

  SHRIKE_MENU_VIEW_RESET,

  SHRIKE_LISTBOX_SHADERS
};

class ShrikeCanvas;

class ShrikeFrame : public wxFrame {
public:
  ShrikeFrame();
  virtual ~ShrikeFrame();

  void quit(wxCommandEvent& event);
  void openModel(wxCommandEvent& event);

  void onShaderSelect(wxCommandEvent& event);
  
  void setShader(Shader* shader);

  void shaderProps(wxCommandEvent& event);
  void showVsh(wxCommandEvent& event);
  void showFsh(wxCommandEvent& event);
  void reinit(wxCommandEvent& event);

  void resetView(wxCommandEvent& event);
  
private:
  void showProgram(const SH::ShProgram& program,
                   std::string name);

  wxListBox* initShaderList(wxWindow* parent);

  ShrikeCanvas* m_canvas;
  UniformPanel* m_panel;

  Shader* m_shader;
  
  DECLARE_EVENT_TABLE()
};

#endif
