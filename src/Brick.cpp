#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;


class Brick : public Shader {
public:
  Brick();
  ~Brick();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Brick instance;
};

Brick::Brick()
  : Shader("Tiling: Brick")
{
}

Brick::~Brick()
{
}

bool Brick::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;

  ShColor3f SH_DECL(brick) = ShColor3f(0.7, 0.1, 0.1);
  ShColor3f SH_DECL(mortar) = ShColor3f(0.3, 0.3, 0.3);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(3.0, 5.0);
  scale.name("brick size");
  scale.range(1.0, 100.0);

  ShAttrib2f SH_DECL(mortarsize) = ShAttrib2f(0.03, 0.03);
  mortarsize.name("mortar size");
  mortarsize.range(0.0, 0.1);

  ShAttrib1f SH_DECL(offset) = ShAttrib1f(0.5);
  offset.range(0.0, 1.0);
 
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords
    
    ShOutputColor3f result;

    tc *= scale;
    
    //tc[0] = cond( fmod(tc(1),2.0) < 1.0, tc(0)+offset, tc(0));
    tc[0] = tc(0) - floor(tc(1))*offset; // change the horizontal position of a line
    tc[1] = tc(1) - floor(tc(1)+0.5);
    tc[0] = tc(0) - floor(tc(0)+0.5);
    ShAttrib1f inside;

    inside = min(abs(tc(0))<0.5-mortarsize(0), abs(tc(1))>mortarsize(1));
    result = cond(inside, brick, mortar);
	    
  } SH_END;
  return true;
}

Brick Brick::instance = Brick();

