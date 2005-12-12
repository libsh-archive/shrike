#ifndef PROJECTTREE_HPP
#define PROJECTTREE_HPP

#include <wx/treectrl.h>
#include "Project.hpp"
#include "Shader.hpp"

class ProjectCommon;

class ProjectTree : public wxTreeCtrl
{
public:
  enum ItemType {
    Root,
    Parent,
    Shader,
    Source,
    Unknown
  };

  ProjectTree(wxWindow* parent, wxWindowID id, 
    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
    long style = wxTR_HAS_BUTTONS);

  ItemType get_item_type(wxTreeItemId) const;
  Project* get_project(wxTreeItemId item);
  ::Shader* get_shader(wxTreeItemId item);
  const wxString& get_source(wxTreeItemId item);

  void insert(Project* project);
  void remove();

  void update();
private:
  void update(ProjectCommon* common);

  int m_folder_icon;
  int m_shader_icon;
  int m_source_icon;
};

#endif
