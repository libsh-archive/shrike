#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

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

ShProgram doText(const std::string& text)
{
  float px = 0.0;
  float py = 0.0;
  float sep = 7.0;
  float lineheight = 60.0;
  float linesep = 20.0;
  ShProgram phrase = SH_BEGIN_PROGRAM() { ShOutputAttrib1f result = 0.0; } SH_END;
  for (std::string::const_iterator c = text.begin(); c != text.end(); c++) {
    ShProgram letter = SH_BEGIN_PROGRAM() { ShOutputAttrib1f result = 0.0; } SH_END;
    switch (*c) {
    case 'H':
      {
        float vw = 13.0;
        float vh = 60.0;
        float hw = 27.0;
        float hy = 24.0;
        float hh = 11.0;
        letter = u(u(rect(px, py, vw, vh),
                   rect(px + vw, py + hy, hw, hh)),
                 rect(px + vw + hw, py, vw, vh));

        px += vw + hw + vw;
      }
      break;
    case 'S':
      {
        float ro = 18.0;
        float w = 12.0;
        float ri = ro - w;
        letter = u(s(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                   rect(px, py + ro, ro, ro)),
                 s(s(circ(px + ro, py + ro + ro + ri, ro), circ(px + ro, py + ro + ro + ri, ri)),
                   rect(px + ro, py + ro + ri, ro, ro)));
        px += ro + ro;
      }
      break;
    case 'W':
      {
        float vw = 13.0;
        float vh = 60.0;
        float w1 = 17.0;
        letter = srect(px + w1, py, vw, vh, -w1);
        letter = u(letter, srect(px + w1, py, vw, vh, w1));
        letter = u(letter, srect(px + w1 + w1 + w1, py, vw, vh, -w1));
        letter = u(letter, srect(px + w1 + w1 + w1, py, vw, vh, w1));

        px += vw + w1 * 4.0;
      }
      break;
    case 'a':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        letter = u(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                 rect(px + ro + ri, py, ro - ri, ro + ro));
        px += ro + ro;
      }
      break;
    case 'b':
      {
        float vh = 60.0;
        float ro = 22.5;
        float ri = ro - 12.0;
        letter = u(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                 rect(px, py, ro - ri, vh));
        px += ro + ro;
      }
      break;
    case 'd':
      {
        float vh = 60.0;
        float ro = 22.5;
        float ri = ro - 12.0;
        letter = u(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                 rect(px + ro + ri, py, ro - ri, vh));
        px += ro + ro;
      }
      break;
    case 'e':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        float eh = 8.0;
        letter = u(s(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                   rect(px + ro, py + ro - ri + ri/2.0, ro, ri/2.0 - eh/2.0)),
                 rect(px + (ro-ri), py + ro - eh/2.0, ri * 2.0, eh));
        px += ro + ro;
      }
      break;
    case 'i':
      {
        float ro = 22.5;
        float vw = 13.0;
        float w = vw/2.0;
        float vh = 60.0;
        letter = u(rect(px, py, vw, ro + ro),
                   circ(px + w, py + ro + ro + w + w, w));

        px += vw;
      }
      break;
    case 'h':
      {
        float vh = 60.0;
        float ro = 22.5;
        float w = 12.0;
        float ri = ro - w;
        letter = u(u(s(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                     rect(px, py, ro + ro, ro)),
                   rect(px + ro + ri, py, w, ro)),
                 rect(px, py, w, vh));
        px += ro + ro;
      }
      break;
    case 'n':
      {
        float ro = 22.5;
        float w = 12.0;
        float ri = ro - w;
        letter = u(u(s(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                     rect(px, py, ro + ro, ro)),
                   rect(px + ro + ri, py, w, ro)),
                 rect(px, py, w, ro+ro));
        px += ro + ro;
      }
      break;
    case 'l':
      {
        float vw = 13.0;
        float vh = 60.0;
        letter = rect(px, py, vw, vh);

        px += vw;
      }
      break;
    case 'o':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        letter = s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri));
        px += ro + ro;
      }
      break;
    case 'r':
      {
        float ro = 22.5;
        float ri = ro - 12.0;
        letter = u(s(s(circ(px + ro, py + ro, ro), circ(px + ro, py + ro, ri)),
                   rect(px, py, ro + ro, ro)),
                 rect(px, py, ro - ri, ro + ro));
        px += ro + ro;
      }
      break;
    case 'v':
      {
        float vw = 6.5;
        float vh = 45.0;
        float w1 = vw;
        letter = srect(px + w1, py, vw, vh, -w1);
        letter = u(letter, srect(px + w1, py, vw, vh, w1));

        px += vw + w1 + w1;
      }
      break;
    case ' ':
      px += 21.0;
      break;
    case '\n':
      px = -sep;
      py -= lineheight + linesep;
      break;
    default:
      break;
    }
    phrase = u(phrase, letter);
    px += sep;
  }
  return phrase;
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
  
  ShProgram texter = (doText(m_text) >> posn) << scaler;

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

