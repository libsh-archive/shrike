#ifndef SHRIKEFRAME_HPP
#define SHRIKEFRAME_HPP

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/minifram.h>

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
  SHRIKE_MENU_VIEW_SCREENSHOT,
  SHRIKE_MENU_VIEW_BACKGROUND,
  SHRIKE_MENU_VIEW_FULLSCREEN,
  SHRIKE_MENU_VIEW_FPS,

  SHRIKE_TREECTRL_SHADERS
};

class ShrikeCanvas;
class wxSplitterWindow;

class ShrikeFrame : public wxFrame {
public:
  ShrikeFrame();
  virtual ~ShrikeFrame();

  void quit(wxCommandEvent& event);
  void openModel(wxCommandEvent& event);

  void onShaderSelect(wxTreeEvent& event);
  
  void setShader(Shader* shader);

  void shaderProps(wxCommandEvent& event);
  void showVsh(wxCommandEvent& event);
  void showFsh(wxCommandEvent& event);
  void reinit(wxCommandEvent& event);

  void resetView(wxCommandEvent& event);
  void setBackground(wxCommandEvent& event);
  void fullscreen(wxCommandEvent& event);
  void screenshot(wxCommandEvent& event);
  void setFullscreen(bool);
  void fps(wxCommandEvent& event);
  void setFps(bool);

  void keyDown(wxKeyEvent& event);

  static ShrikeFrame* instance();
  
private:
  void showProgram(SH::ShProgram program,
                   std::string name);

  wxTreeCtrl* initShaderList(wxWindow* parent);

  ShrikeCanvas* m_canvas;
  UniformPanel* m_panel;
  wxFrame* m_preview;
  wxSplitterWindow* m_hsplitter;
  wxSplitterWindow* m_right_window;

  wxTreeCtrl* m_shaderList;
  Shader* m_shader;

  bool m_fullscreen;
  bool m_fps;

  static ShrikeFrame* m_instance;
  DECLARE_EVENT_TABLE()
};

#endif
