#include "ShrikePropsDialog.hpp"
#include <iostream>

class StdStringValidator : public wxTextValidator {
public:
  StdStringValidator(long style, std::string& str)
    : wxTextValidator(style, (m_wxString = new wxString(str.c_str()))),
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
    m_string = *m_wxString;
    return true;
  }

  virtual bool TransferToWindow()
  {
    *m_wxString = m_string.c_str();
    return wxTextValidator::TransferToWindow();
  }
  
private:
  std::string& m_string;
  wxString* m_wxString;
};

ShrikePropsDialog::ShrikePropsDialog(wxWindow* parent, Shader* shader)
  : wxDialog(parent, -1, "Shader Properties", wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODAL)
{
  Shader::StringParamList::iterator I;
  wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 2, 2);
  sizer->AddGrowableCol(1);
  SetAutoLayout(true);
  SetSizer(sizer);
  
  for (I = shader->beginStringParams(); I != shader->endStringParams(); ++I) {
    wxStaticText* label = new wxStaticText(this, -1, I->name.c_str());
    wxTextCtrl* ctrl = new wxTextCtrl(this, -1, I->param.c_str(),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      StdStringValidator(wxFILTER_NONE, I->param));

    sizer->Add(label);
    sizer->Add(ctrl);
  }

  wxButton* btn_ok = new wxButton(this, wxID_OK, "O&K");
  sizer->Add(btn_ok);
  btn_ok->SetDefault();
  
  sizer->Layout();
  sizer->SetSizeHints(this);
  sizer->Fit(this);
}
