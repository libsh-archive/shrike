#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <list>
#include <sh/sh.hpp>

class Shader {
public:
  Shader(const std::string& name);
  virtual ~Shader();
  
  virtual bool init() = 0;
  virtual void bind() = 0;

  virtual SH::ShProgram fragment() = 0;
  virtual SH::ShProgram vertex() = 0;
  
  const std::string& name() const;

  typedef std::list<Shader*> list;
  typedef list::iterator iterator;
  typedef list::const_iterator const_iterator;

  static iterator begin();
  static iterator end();
  static const_iterator begin_const();
  static const_iterator end_const();
  
private:
  std::string m_name;

  static list* getList();
  
  static void append(Shader* shader);
  static list* m_list;
};

#endif
