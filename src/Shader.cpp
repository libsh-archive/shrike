#include "Shader.hpp"

Shader::Shader(const std::string& name)
  : m_name(name)
{
  append(this);
}

Shader::~Shader()
{
}

const std::string& Shader::name() const
{
  return m_name;
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
