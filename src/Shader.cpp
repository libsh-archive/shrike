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
#include "Shader.hpp"
//#include "ShrikeCanvas.hpp"

Shader::Shader(const std::string& name, const Globals &globals)
  : m_globals(globals),
    m_name(name),
    m_has_been_init(false),
    m_failed(false),
    m_shaders(0)
{
}

Shader::~Shader()
{
  delete m_shaders;
}

void Shader::set_failed(bool failed)
{
  m_failed = failed;
}

bool Shader::failed() const
{
  return m_failed;
}

bool Shader::firstTimeInit()
{
  if (m_has_been_init) return true;
  bool success = init();
  m_has_been_init = true;
  return success;
}

void Shader::bind() {
  if (!m_shaders) {
    m_shaders = new SH::ShProgramSet(vertex(), fragment());
  }
  SH::shBind(*m_shaders);
}

const std::string& Shader::name() const
{
  return m_name;
}

std::size_t Shader::paramCount() const
{
  return m_stringParams.size();
}

bool Shader::render(const ShUtil::ShObjMesh& mesh)
{
  return false;
}

Shader::StringParamList::iterator Shader::beginStringParams()
{
  return m_stringParams.begin();
}

Shader::StringParamList::iterator Shader::endStringParams()
{
  return m_stringParams.end();
}
/*
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
*/
void Shader::setStringParam(const std::string& name,
                            std::string& param)
{
  m_stringParams.push_back(StringParam(name, param));
}
/*
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
*/

ShaderList &GetShaders()
{
  static ShaderList shaders;
  return shaders;
}
