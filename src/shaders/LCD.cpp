#include <sh/sh.hpp>
#include <iostream>
#include <cstdarg>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;

class LCD : public Shader {
public:
  LCD();
  ~LCD();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
  
  static LCD instance;
};

LCD::LCD()
  : Shader("LCD")
{
}

LCD::~LCD()
{
}

template<int N>
ShAttrib<N, SH_CONST> construct(double a, ...)
{
  va_list ap;
  va_start(ap, a);
  float args[N];

  args[0] = a;
  for (int i = 1; i < N; i++) {
    args[i] = va_arg(ap, double);
  }
  
  va_end(ap);
  
  return ShAttrib<N, SH_CONST>(args);
}

ShAttrib1f lcd(const ShTexCoord2f& tc,
               ShAttrib1f number,
               int digits = 3,
               bool showgrid = false)
{
  float w = 0.2;
  float h = 0.5;
  float t = 0.02;

  ShAttrib<7, SH_CONST> segments[10] = {
    //           TT   LT   RT   CE   LB   RB   BB  
    construct<7>(1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0), // 0
    construct<7>(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0), // 1
    construct<7>(1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0), // 2
    construct<7>(1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0), // 3
    construct<7>(0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0), // 4
    construct<7>(1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0), // 5
    construct<7>(1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0), // 6
    construct<7>(1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0), // 7
    construct<7>(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0), // 8
    construct<7>(1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0)};// 9
  
  ShAttrib<7, SH_CONST> posns[4] = {
    construct<7>(0.0  , 0.0  , w - t, t          , 0.0  , w - t  , 0.0), // left
    construct<7>(w    , t    , w    , w - t      , t    , w      , w  ), // right
    construct<7>(h - t, h/2.0, h/2.0, (h - t)/2.0, 0.0  , 0.0    , 0.0), // bottom
    construct<7>(h    , h    , h    , (h + t)/2.0, h/2.0, h/2.0  , t  )};  // top
  
  float eps = 0.01;
  
  ShAttrib1f result(0.0f);
  ShTexCoord2f loc = tc;

  loc(0) -= (w + t)*digits;
  for (int i = 0; i < digits; i++) {
    ShAttrib1f digit = floor(number) - floor(number / 10.0f)*10.0f;
    
    ShAttrib<7, SH_TEMP> in[4];

    in[0] = posns[0] < loc(0);
    in[1] = posns[1] > loc(0);
    in[2] = posns[2] < loc(1);
    in[3] = posns[3] > loc(1);

    in[0] = in[0] * in[1] * in[2] * in[3];

    for (int d = 0; d < 10; d++) {
      result += (abs(digit - (float)d) < eps) * dot(segments[d], in[0]);
    }
    
    if (showgrid) {
      result += (dot(fillcast<7>(ShConstAttrib1f(0.1)), in[0]) > 0.0) * 0.2;
    }

    loc(0) += w + t;
    number /= 10.0f;
  }
  return result;
}

bool LCD::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
  } SH_END;

  ShAttrib1f SH_DECL(value);
  value.range(0.0, 999.9);

  ShColor3f SH_DECL(background) = ShColor3f(0.69, 0.75, 0.68);
  ShColor3f SH_DECL(foreground) = ShColor3f(0.18, 0.20, 0.18);

  ShVector2f SH_DECL(offset) = ShVector2f(0.0, 0.25);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);

    tc(1) = 1.0 - tc(1);
    result = cond(lcd(tc - offset, value), foreground, background);
  } SH_END;
  return true;
}

LCD LCD::instance = LCD();


