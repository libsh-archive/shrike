// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#include "Shader.hpp"
#include "ShrikeCanvas.hpp"

Shader::Shader(const std::string& name)
  : m_name(name),
    m_has_been_init(false),
    m_failed(false)
{
  append(this);
}

Shader::~Shader()
{
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
  SH::ShProgram vsh = vertex();
  SH::ShProgram fsh = fragment();
  SH::shBind(vsh);
  SH::shBind(fsh);
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
