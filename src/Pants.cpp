#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Pants : public Shader {
public:
  Pants();
  ~Pants();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Pants instance;
};

Pants::Pants()
  : Shader("Dad's Pants")
{
}

Pants::~Pants()
{
}

bool Pants::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(.2, 0.5, 0.9);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0, 1.0);
  scale.range(1.0, 1000.0);

  ShAttrib1f SH_DECL(power) = 12.0;
  power.range(0.0, 32.0);

  // This simple but cool Moire pattern function is due to Craig Kaplan.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords

    ShOutputColor3f result;

    tc *= scale;
    ShAttrib1f d = dot(tc,tc);
    ShAttrib1f r = frac(d * pow(2.0, power));
    
    result = r * color;
  } SH_END;
  return true;
}

Pants Pants::instance = Pants();


