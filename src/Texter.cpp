#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Hello : public Shader {
public:
  Hello();
  ~Hello();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static Hello* instance;
};

Hello::Hello()
  : Shader("Hello")
{
}

Hello::~Hello()
{
}


ShTexCoord2f posn;

ShProgram u(const ShProgram& a, const ShProgram& b)
{
  return max<ShAttrib1f>() << (a & b);
}

ShProgram i(const ShProgram& a, const ShProgram& b)
{
  return min<ShAttrib1f>() << (a & b);
}

ShProgram s(const ShProgram& a, const ShProgram& b)
{
  return i(a, sub<ShAttrib1f>() << ShConstant1f(1.0) << b);
}

ShProgram rect(float x, float y,
               float w, float h)
{
  ShProgram res = SH_BEGIN_PROGRAM() {
    ShVector2f o = posn - ShConstant2f(x, y);
    ShOutputAttrib1f result = min((o(0) > 0.0)*(o(1) > 0.0), (o(0) < w)*(o(1) < h));
  } SH_END;
  return res;
}

ShProgram srect(float x, float y,
                float w, float h,
                float skew)
{
  ShProgram res = SH_BEGIN_PROGRAM() {
    ShVector2f o = posn - ShConstant2f(x, y);
    o(0) -= o(1) * (skew/h);
    ShOutputAttrib1f result = min((o(0) > 0.0)*(o(1) > 0.0), (o(0) < w)*(o(1) < h));
  } SH_END;
  return res;
}

ShProgram circ(float x, float y, float r)
{
  ShProgram res = SH_BEGIN_PROGRAM() {
    ShVector2f o = posn - ShConstant2f(x, y);
    ShOutputAttrib1f result = ((o | o) < (r*r));
  } SH_END;
  return res;
}

ShProgram doText(const char* str)
{
  float px = 0.0;
  float sep = 7.0;
  ShProgram cur = SH_BEGIN_PROGRAM() { ShOutputAttrib1f result = 0.0; } SH_END;
  for (const char* c = str; *c; c++) {
    ShProgram step = SH_BEGIN_PROGRAM() { ShOutputAttrib1f result = 0.0; } SH_END;
    switch (*c) {
    case 'H':
      {
        float vw = 13.0;
        float vh = 60.0;
        float hw = 27.0;
        float hy = 24.0;
        float hh = 11.0;
        step = u(u(rect(px, 0.0, vw, vh),
                   rect(px + vw, hy, hw, hh)),
                 rect(px + vw + hw, 0.0, vw, vh));

        px += vw + hw + vw;
      }
      break;
    case 'S':
      {
        float ro = 18.0;
        float w = 12.0;
        float ri = ro - w;
        step = u(s(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                   rect(px, ro, ro, ro)),
                 s(s(circ(px + ro, ro + ro + ri, ro), circ(px + ro, ro + ro + ri, ri)),
                   rect(px + ro, ro + ri, ro, ro)));
        px += ro + ro;
      }
      break;
    case 'W':
      {
        float vw = 13.0;
        float vh = 60.0;
        float w1 = 17.0;
        step = srect(px + w1, 0.0, vw, vh, -w1);
        step = u(step, srect(px + w1, 0.0, vw, vh, w1));
        step = u(step, srect(px + w1 + w1 + w1, 0.0, vw, vh, -w1));
        step = u(step, srect(px + w1 + w1 + w1, 0.0, vw, vh, w1));

        px += vw + w1 * 4.0;
      }
      break;
    case 'a':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        step = u(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                 rect(px + ro + ri, 0.0, ro - ri, ro + ro));
        px += ro + ro;
      }
      break;
    case 'b':
      {
        float vh = 60.0;
        float ro = 22.5;
        float ri = ro - 12.0;
        step = u(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                 rect(px, 0.0, ro - ri, vh));
        px += ro + ro;
      }
      break;
    case 'd':
      {
        float vh = 60.0;
        float ro = 22.5;
        float ri = ro - 12.0;
        step = u(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                 rect(px + ro + ri, 0.0, ro - ri, vh));
        px += ro + ro;
      }
      break;
    case 'e':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        float eh = 8.0;
        step = u(s(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                   rect(px + ro, ro - ri + ri/2.0, ro, ri/2.0 - eh/2.0)),
                 rect(px + (ro-ri), ro - eh/2.0, ri * 2.0, eh));
        px += ro + ro;
      }
      break;
    case 'h':
      {
        float vh = 60.0;
        float ro = 22.5;
        float w = 12.0;
        float ri = ro - w;
        step = u(u(s(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                     rect(px, 0.0, ro + ro, ro)),
                   rect(px + ro + ri, 0.0, w, ro)),
                 rect(px, 0.0, w, vh));
        px += ro + ro;
      }
      break;
    case 'n':
      {
        float ro = 22.5;
        float w = 12.0;
        float ri = ro - w;
        step = u(u(s(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                     rect(px, 0.0, ro + ro, ro)),
                   rect(px + ro + ri, 0.0, w, ro)),
                 rect(px, 0.0, w, ro+ro));
        px += ro + ro;
      }
      break;
    case 'l':
      {
        float vw = 13.0;
        float vh = 60.0;
        step = rect(px, 0.0, vw, vh);

        px += vw;
      }
      break;
    case 'o':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        step = s(circ(px + ro, ro, ro), circ(px + ro, ro, ri));
        px += ro + ro;
      }
      break;
    case 'r':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        step = u(s(s(circ(px + ro, ro, ro), circ(px + ro, ro, ri)),
                   rect(px, 0.0, ro + ro, ro)),
                 rect(px, 0.0, ro - ri, ro + ro));
        px += ro + ro;
      }
      break;
    case ' ':
      px += 21.0;
    default:
      break;
    }
    cur = u(cur, step);
    px += sep;
  }
  return cur;
}

bool Hello::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(100.0, 100.0);
  scale.range(10.0, 500.0);
  ShAttrib2f SH_DECL(trans) = ShAttrib2f(0.0, 0.0);
  trans.range(-500.0, 500.0);

  ShProgram scaler = SH_BEGIN_PROGRAM() {
    ShInOutTexCoord2f tc;
    tc(1) = 1.0 - tc(1);
    tc *= scale;
    tc += trans;
  } SH_END;
  
  ShProgram texter = (doText("Shade") >> posn) << scaler;

  ShProgram renderer = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputAttrib1f in;
    ShOutputColor3f out = cond(in,
                               ShColor3f(0.0, 0.0, 0.0),
                               ShColor3f(1.0, 1.0, 1.0));
  } SH_END;
  
  fsh = renderer << texter;
  
  return true;
}

void Hello::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

Hello* Hello::instance = new Hello();

