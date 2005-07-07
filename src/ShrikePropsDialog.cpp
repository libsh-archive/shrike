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
#include "ShrikePropsDialog.hpp"
#include <iostream>

class StdStringValidator : public wxTextValidator {
public:
  StdStringValidator(long style, std::string& str)
    : wxTextValidator(style, (m_wxString = new wxString( wxConvLibc.cMB2WX( str.c_str() ) )) ),
      m_string(str)
  {
  }

  ~StdStringValidator()
  {
    delete m_wxString;
  }

  StdStringValidator(const StdStringValidator& v)
    : wxTextValidator(v.GetStyle(),
                      m_wxString = new wxString(*v.m_wxString)),
      m_string(v.m_string)
  {
  }

  virtual wxValidator* Clone() const
  {
    StdStringValidator* v = new StdStringValidator(*this);
    return v;
  }
  
  virtual bool Validate(wxWindow* parent)
  {
    return wxTextValidator::Validate(parent);
  }
  
  virtual bool TransferFromWindow()
  {
    if (!wxTextValidator::TransferFromWindow()) return false;
    m_string = wxConvLibc.cWX2MB(*m_wxString);
    return true;
  }

  virtual bool TransferToWindow()
  {
    *m_wxString = wxConvLibc.cMB2WX(m_string.c_str());
    return wxTextValidator::TransferToWindow();
  }
  
private:
  std::string& m_string;
  wxString* m_wxString;
};

ShrikePropsDialog::ShrikePropsDialog(wxWindow* parent, Shader* shader)
  : wxDialog(parent, -1, wxT("Shader Properties"), wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODAL)
{
  Shader::StringParamList::iterator I;
  wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 2, 2);
  sizer->AddGrowableCol(1);
  SetAutoLayout(true);
  SetSizer(sizer);
  
  for (I = shader->beginStringParams(); I != shader->endStringParams(); ++I) {
    wxStaticText* label = new wxStaticText(this, -1, wxConvLibc.cMB2WX(I->name.c_str()));
    wxTextCtrl* ctrl = new wxTextCtrl(this, -1, wxConvLibc.cMB2WX(I->param.c_str()),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      StdStringValidator(wxFILTER_NONE, I->param));

    sizer->Add(label);
    sizer->Add(ctrl);
  }

  wxButton* btn_ok = new wxButton(this, wxID_OK, wxT("O&K") );
  sizer->Add(btn_ok);
  btn_ok->SetDefault();
  
  sizer->Layout();
  sizer->SetSizeHints(this);
  sizer->Fit(this);
}
