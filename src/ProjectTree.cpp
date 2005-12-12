#include "ProjectTree.hpp"
#include <wx/artprov.h>

struct ProjectCommon
{
  ProjectCommon(Project* project, wxTreeItemId root)
    : m_project(project), m_root(root)
  {}
  ~ProjectCommon() { delete m_project; }

  Project* m_project;
  wxTreeItemId m_root;
};

struct ProjectItem : public wxTreeItemData
{
  ProjectItem(ProjectCommon* common, ProjectTree::ItemType type) 
    : m_common(common), m_type(type) {}
  ~ProjectItem() 
  { 
    if (type() == ProjectTree::Root) delete m_common; 
  }

  ProjectTree::ItemType type() const { return m_type; }
  ProjectCommon* common() { return m_common; }
private:
  ProjectCommon* m_common;
  ProjectTree::ItemType m_type;
};

struct ShaderItem : public ProjectItem
{
  ShaderItem(ProjectCommon* common, ::Shader* shader) 
    : ProjectItem(common, ProjectTree::Shader), m_shader(shader)
  {}

  ::Shader* shader() { return m_shader; }
private:
  ::Shader* m_shader;
};

struct SourceItem : public ProjectItem
{
  SourceItem(ProjectCommon* common, const wxString &name) 
    : ProjectItem(common, ProjectTree::Source), m_name(name) 
  {}

  const wxString& name() const { return m_name; }
private:
  wxString m_name;
};

ProjectTree::ProjectTree(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
  const wxSize& size, long style)
  : wxTreeCtrl(parent, id, pos, size, style)
{
  wxImageList* images = new wxImageList(16,16);
  wxIcon icon = wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU, wxSize(16,16));
  m_folder_icon = images->Add(icon);
  icon = wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU, wxSize(16,16));
  m_shader_icon = images->Add(icon);
  icon = wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU, wxSize(16,16));
  m_source_icon = images->Add(icon);

  AssignImageList(images);
}

Project* ProjectTree::get_project(wxTreeItemId item)
{
  ProjectItem* data = dynamic_cast<ProjectItem*>(GetItemData(item));
  if (data)
    return data->common()->m_project;
  return 0;
}

ProjectTree::ItemType ProjectTree::get_item_type(wxTreeItemId item) const
{
  ProjectItem* data = dynamic_cast<ProjectItem*>(GetItemData(item));
  if (data)
    return data->type();
  return ProjectTree::Unknown;
}

Shader* ProjectTree::get_shader(wxTreeItemId item)
{
  ShaderItem* data = dynamic_cast<ShaderItem*>(GetItemData(item));
  if (data)
    return data->shader();
  return 0;
}

const wxString& ProjectTree::get_source(wxTreeItemId item)
{
  SourceItem* data = dynamic_cast<SourceItem*>(GetItemData(item));
  if (data)
    return data->name(); 
  return wxT("");
}

void ProjectTree::insert(Project* project)
{
  wxTreeItemId root_id = GetRootItem();
  wxTreeItemId project_id = AppendItem(root_id, project->name());

  ProjectCommon* common = new ProjectCommon(project, project_id);
  SetItemData(project_id, new ProjectItem(common, ProjectTree::Root));

  update(common);
  SelectItem(project_id);
}

void ProjectTree::remove()
{
  ProjectItem* data = dynamic_cast<ProjectItem*>(GetItemData(GetSelection()));
  if (!data)
    return;

  DeleteChildren(data->common()->m_root);
  Delete(data->common()->m_root);
}

void ProjectTree::update()
{
  ProjectItem* data = dynamic_cast<ProjectItem*>(GetItemData(GetSelection()));
  if (!data)
    return;
  update(data->common());
}

void ProjectTree::update(ProjectCommon* common)
{
  Project* project = common->m_project;
  wxTreeItemId root = common->m_root;

  DeleteChildren(root);

  for (ShaderList::iterator I = project->begin_shaders(); I != project->end_shaders(); ++I) { 
    ::Shader* shader = *I;
    
    ShaderItem* item_data = new ShaderItem(common, shader);
    AppendItem(root, wxConvLibc.cMB2WX(shader->name().c_str()), m_shader_icon, -1, item_data);
  }

  wxTreeItemId source = AppendItem(root, wxT("Source"), m_folder_icon);
  SetItemData(source, new ProjectItem(common, ProjectTree::Parent));
  for (Project::FileList::const_iterator I = project->begin_sources(); I != project->end_sources(); ++I) {

    SourceItem* item_data = new SourceItem(common, *I);
    AppendItem(source, *I, m_source_icon, -1, item_data);
  }
}
