#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"
#include "Text.hpp"

using namespace SH;
using namespace ShUtil;

class Texter : public Shader {
public:
  Texter(const std::string&);
  ~Texter();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

private:
  
  std::string m_text;

  static Texter* instance;
};

Texter::Texter(const std::string& text)
  : Shader("\"" + text + "\""),
    m_text(text)
{
  setStringParam("text", m_text);
}

Texter::~Texter()
{
}


bool Texter::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(500.0, 500.0);
  scale.range(100.0, 1000.0);
  ShAttrib2f SH_DECL(trans) = ShAttrib2f(-50.0, -250.0);
  trans.range(-500.0, 500.0);
  
  ShProgram scaler = SH_BEGIN_PROGRAM() {
    ShInOutTexCoord2f tc;
    tc(1) = 1.0 - tc(1);
    tc *= scale;
    tc += trans;
  } SH_END;
  
  ShProgram texter = doText(m_text) << scaler;

  ShProgram renderer = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputAttrib1f in;
    ShOutputColor3f out = cond(in,
                               ShColor3f(0.0, 0.0, 0.0),
                               ShColor3f(1.0, 1.0, 1.0));
  } SH_END;
  
  fsh = renderer << texter;
  
  return true;
}

Texter* Texter::instance = new Texter("Hello World");

