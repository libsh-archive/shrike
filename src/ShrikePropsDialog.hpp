#ifndef SHRIKEPROPSDIALOG_HPP
#define SHRIKEPROPSDIALOG_HPP

#include <wx/wx.h>
#include "Shader.hpp"

class ShrikePropsDialog : public wxDialog {
public:
  ShrikePropsDialog(wxWindow* parent, Shader* shader);
};

#endif
