#ifndef UNIFORMPANEL_HPP
#define UNIFORMPANEL_HPP

#include <list>
#include <wx/wx.h>
#include <wx/sizer.h>
#include "Shader.hpp"

class UniformPanel : public wxScrolledWindow {
public:
  UniformPanel(wxWindow* parent);

  void setShader(Shader* shader);

private:
  wxBoxSizer* m_sizer;
};

#endif
