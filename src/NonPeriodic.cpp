#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class NonPeriodic : public Shader {
public:
  NonPeriodic();
  ~NonPeriodic();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static NonPeriodic instance;
};

NonPeriodic::NonPeriodic()
  : Shader("Nonperiodic tiling")
{
}

NonPeriodic::~NonPeriodic()
{
}

ShAttrib1f do_mod(ShAttrib1f x)
{
  ShAttrib1f dd = floor(x);
  ShAttrib1f diff = x - dd;

  diff = cond(diff > 0.5, diff - 1.0, diff);
  
  return diff;
}

ShAttrib1f aboveLine( ShAttrib1f x0, ShAttrib1f y0, ShAttrib1f m, ShAttrib1f x, ShAttrib1f y )
{
  return (y >= (m*(x-x0) + y0));
}

static const double m = 0.5*(sqrt( 5.0 ) - 1.0);
static const double pm = -1.0 / m;

ShAttrib1f isRed(ShAttrib1f x)
{
  ShAttrib1f mx = do_mod( x );
  ShAttrib1f my = do_mod( m*x - 0.25*sqrt(5.0) );

  // The following conditions amount to checking whether the
  // point (mx,my) lies in one of two polygons inside the
  // unit square.  Each region is actually the intersection of
  // the unit square with a smaller square that partially overlaps
  // it.  That should make for fairly nice tests later if we want.

  // Basically use these four lines to define a tic-tac-toe
  // board, use 3*tb+lr to tell what cell you're in.

  ShAttrib1f lr = 0;
  ShAttrib1f tb = 0;

  lr = cond(aboveLine(-0.5, 1.5, pm, mx, my ), ShConstant1f(2.0),
            cond(aboveLine( -0.5, 0.5, pm, mx, my ), ShConstant1f(1.0), ShConstant1f(0.0)));

  tb = cond(aboveLine( 0.5, 0.5, m, mx, my ), ShConstant1f(2.0),
            cond(aboveLine( -0.5, -0.5, m, mx, my ), ShConstant1f(1.0), ShConstant1f(0.0)));

  ShAttrib1f qq = tb*3.0 + lr;

  return max(abs(qq) < 0.1, abs(qq - 7) < 0.1);
}

bool NonPeriodic::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShColor3f SH_DECL(color1) = ShColor3f(.2, 0.5, 0.9);
  ShColor3f SH_DECL(color2) = ShColor3f(.9, 0.5, 0.2);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(20.0, 20.0);
  scale.range(1.0, 1000.0);

  ShAttrib1f SH_DECL(second) = 0.0;
  second.range(0.0, 1.0);

  // This is a 1D nonperiodic tiling from Craig Kaplan.
  // It should be possible to extend this to make Penrose tiles.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords

    ShOutputColor3f result;

    tc *= scale;
    
    result = cond(max(isRed(tc(0)), second * isRed(tc(1))),
                  color1, color2);
  } SH_END;
  return true;
}

NonPeriodic NonPeriodic::instance = NonPeriodic();


