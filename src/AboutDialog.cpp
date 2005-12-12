#include "AboutDialog.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

AboutDialog::AboutDialog(wxWindow* parent)
  : wxDialog(parent, -1, wxT("About Shrike"), wxDefaultPosition, wxDefaultSize,
      wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

  // TODO: get the version from somewhere
  wxString info_string(
    wxT(
      "\n"
      "Shrike\n"
      "version 0.7.10\n"
      "\n"
      "Copyright 2003-2005 Serious Hack Inc.\n"
    ));
  wxStaticText* info = new wxStaticText(this, -1, info_string,
    wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
  sizer->Add(info, 0, wxEXPAND);
    
  wxString license_string(
    wxT(
    "This program is free software; you can redistribute it and/or "
    "modify it under the terms of the GNU Lesser General Public "
    "License as published by the Free Software Foundation; either "
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "Lesser General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Lesser General Public "
    "License along with this program; if not, write to the Free Software "
    "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA"
    ));
  wxTextCtrl* license = new wxTextCtrl(this, -1, license_string,
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
  sizer->Add(license, 1, wxEXPAND);

  wxButton* button = new wxButton(this, wxID_OK, wxT("Ok"));
  sizer->Add(button, 0, wxEXPAND);

  SetSizer(sizer);
}
