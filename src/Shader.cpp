#include "Shader.hpp"
#include "ShrikeCanvas.hpp"

Shader::Shader(const std::string& name)
  : m_name(name),
    m_has_been_init(false)
{
  append(this);
}

Shader::~Shader()
{
}

bool Shader::firstTimeInit()
{
  if (m_has_been_init) return true;
  bool success = init();
  m_has_been_init = true;
  return success;
}

void Shader::bind() {
  SH::ShProgram vsh = vertex();
  SH::ShProgram fsh = fragment();
  std::cerr << "Binding " << name() << std::endl;
  SH::shBindShader(vsh);
  SH::shBindShader(fsh);
}

const std::string& Shader::name() const
{
  return m_name;
}

std::size_t Shader::paramCount() const
{
  return m_stringParams.size();
}

void Shader::render()
{
  ShrikeCanvas::instance()->renderObject();
}

Shader::StringParamList::iterator Shader::beginStringParams()
{
  return m_stringParams.begin();
}

Shader::StringParamList::iterator Shader::endStringParams()
{
  return m_stringParams.end();
}

Shader::iterator Shader::begin()
{
  return getList()->begin();
}

Shader::iterator Shader::end()
{
  return getList()->end();
}

Shader::const_iterator Shader::begin_const()
{
  return getList()->begin();
}

Shader::const_iterator Shader::end_const()
{
  return getList()->end();
}

void Shader::setStringParam(const std::string& name,
                            std::string& param)
{
  m_stringParams.push_back(StringParam(name, param));
}

void Shader::append(Shader* shader)
{
  getList()->push_back(shader);
}

Shader::list* Shader::getList()
{
  if (!m_list) {
    m_list = new list;
  }
  return m_list;
}

Shader::list* Shader::m_list = 0;
