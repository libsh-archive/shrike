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
#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <list>
#include <sh/sh.hpp>

class Shader {
public:
  Shader(const std::string& name);
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
  
  virtual void render();

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
  
  typedef std::list<Shader*> list;
  typedef list::iterator iterator;
  typedef list::const_iterator const_iterator;

  static iterator begin();
  static iterator end();
  static const_iterator begin_const();
  static const_iterator end_const();
  
protected:
  void setStringParam(const std::string& name,
                      std::string& param);
  
private:
  std::string m_name;

  bool m_has_been_init;

  bool m_failed;

  StringParamList m_stringParams;

  static list* getList();
  
  static void append(Shader* shader);
  static list* m_list;
};

#endif
