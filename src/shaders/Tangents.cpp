#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Tangents : public Shader {
public:
  Tangents();
  ~Tangents();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static Tangents instance;
};

Tangents::Tangents()
  : Shader("Debugging: Tangents")
{
}

Tangents::~Tangents()
{
}

bool Tangents::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp);
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  vsh = shSwizzle("posh") << vsh;
  ShProgram keeper = SH_BEGIN_PROGRAM() {
    ShInOutVector3f tan; // Pass through untransformed tangents
  } SH_END_PROGRAM;
 vsh = vsh & keeper;
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputVector3f i;
    ShOutputColor3f o = i * 0.5f + 0.5f;
  } SH_END_PROGRAM;
  return true;
}


Tangents Tangents::instance = Tangents();

