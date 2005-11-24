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
#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <list>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>

struct Globals;

class Shader {
public:
  Shader(const std::string& name, const Globals &globals);
  virtual ~Shader();

  /// Set this to true if the shader has failed to be initialized, so
  /// it won't be initialized/bound again
  void set_failed(bool failed);
  /// Return whether this shader has failed to initialize.
  bool failed() const;
  
  virtual bool init() = 0;
  virtual void bind(); // binds vertex() and fragment()

  virtual SH::ShProgram fragment() = 0;
  virtual SH::ShProgram vertex() = 0;
  
  virtual bool render(const ShUtil::ShObjMesh&);

  bool firstTimeInit();
  
  const std::string& name() const;

  std::size_t paramCount() const;

  struct StringParam {
    StringParam(const std::string& name,
                std::string& param)
      : name(name), param(param)
    {
    }
    
    std::string name;
    std::string& param;
  };
  
  typedef std::list<StringParam> StringParamList;

  StringParamList::iterator beginStringParams();
  StringParamList::iterator endStringParams();
/* 
  typedef std::list<Shader*> list;
  typedef list::iterator iterator;
  typedef list::const_iterator const_iterator;
  static iterator begin();
  static iterator end();
  static const_iterator begin_const();
  static const_iterator end_const();
*/
protected:
  void setStringParam(const std::string& name,
                      std::string& param);
  
  const Globals &m_globals;
private:
  std::string m_name;

  bool m_has_been_init;

  bool m_failed;

  StringParamList m_stringParams;

  SH::ShProgramSet* m_shaders;
/*
  static list* getList();
  
  static void append(Shader* shader);
  static list* m_list;
*/
};

typedef std::list<Shader*> ShaderList;
ShaderList &GetShaders();

#include "Globals.hpp"
template <class T>
struct StaticLinkedShader {
  StaticLinkedShader() : shader(new T(GetGlobals())) {
    GetShaders().push_back(shader);
  }
  T *shader;
};

#endif
