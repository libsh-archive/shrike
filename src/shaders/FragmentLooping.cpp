#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class FragmentLooping : public Shader {
public:
  FragmentLooping();
  ~FragmentLooping();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static FragmentLooping instance;
};

FragmentLooping::FragmentLooping()
  : Shader("Branching: Loops in Fragment Unit")
{
}

FragmentLooping::~FragmentLooping()
{
}

bool FragmentLooping::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("texcoord", "posh") << vsh;

  
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(julia_max_iter) = 20.0;
  julia_max_iter.range(1.0, 40.0);
  ShAttrib2f SH_DECL(julia_c) = ShAttrib2f(0.54, -0.51);
  julia_c.range(-1.0, 1.0);
  ShAttrib1f SH_DECL(brightness) = 1.0;
  brightness.range(0.0, 10.0);
  ShAttrib1f SH_DECL(height) = 0.2;
  height.range(0.0, 4.0);

  ShAttrib1f SH_DECL(zoom) = 0.33;
  zoom.range(0.0, 10.0);

  ShPoint2f SH_DECL(centre) = ShPoint2f(0.0, 0.0);
  centre.range(-2.0, 2.0);
  
  ShAttrib1f SH_DECL(gamma) = 0.7;
  gamma.range(0.0, 1.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f u;
    
    ShOutputColor3f ocol; // Color of result

    u = u - centre;
    u /= zoom;
    
    ShAttrib1f i = 0.0; 
    SH_WHILE((dot(u, u) < 2.0) * (i < julia_max_iter)) {
      ShTexCoord2f v;
      v(0) = u(0)*u(0) - u(1)*u(1);
      v(1) = 2.0 * u(0) * u(1);
      u = v + julia_c;
      i += 1.0;
    } SH_ENDWHILE;

    ShAttrib1f disp = pow(i / julia_max_iter, gamma);
    ocol = disp * diffuse * brightness;
  } SH_END;
  
  return true;
}

FragmentLooping FragmentLooping::instance = FragmentLooping();

