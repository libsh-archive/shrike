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
  : Shader("Vector Graphics: CSG Text: \"" + text + "\""),
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

