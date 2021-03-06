// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////
#include <sh/sh.hpp>
#include <iostream>
#include <cstdarg>
#include "Shader.hpp"
#include "Globals.hpp"
#include "LCD.hpp"

using namespace SH;

LCD::LCD(const Globals &globals)
  : Shader("LCD", globals)
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

ShAttrib1f lcd(const ShTexCoord2f& tc, ShAttrib1f number,
               int intDigits, int fracDigits, bool showgrid, bool handleneg)
{
  float w = 0.2;
  float h = 0.5;
  float t = 0.02;
  float eps = 0.01;

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
    construct<7>(0.0  , 0.0  , w - t, t          , 0.0  , w - t  , 0.0),  // left
    construct<7>(w    , t    , w    , w - t      , t    , w      , w  ),  // right
    construct<7>(h - t, h/2.0, h/2.0, (h - t)/2.0, 0.0  , 0.0    , 0.0),  // bottom
    construct<7>(h    , h    , h    , (h + t)/2.0, h/2.0, h/2.0  , t  )}; // top
  
  ShAttrib1f result(0.0f);

  ShTexCoord2f loc = tc;
  ShAttrib1f f = floor(loc(0) / (w + t));
  ShAttrib1f index = f - intDigits + 1; 
  
  number = number * pow(10.0, index);

  loc(0) = loc(0) - f * (w+t);
  ShAttrib1f digit = floor(number) - floor(number / 10.0f)*10.0f;

  if (handleneg) {
    digit = cond(number < 0.0, 9.0 - digit, digit);
  }

  // digit = abs(index); // Useful for debugging
  
  ShAttrib<7, SH_TEMP> in[4];
  
  in[0] = posns[0] < loc(0);
  in[1] = posns[1] > loc(0);
  in[2] = posns[2] < loc(1);
  in[3] = posns[3] > loc(1);
  
  in[0] = in[0] * in[1] * in[2] * in[3];
  
  for (int d = 0; d < 10; d++) {
    result += (abs(digit - (float)d) < eps) * dot(segments[d], in[0]);
  }

  result *= (index < fracDigits + 1 + eps); 
  
  if (handleneg) {
    result = cond(index < -0.9 && abs(number/10.0) < 0.1,
                  ShConstAttrib1f(0.0f), result);
    result = cond(number < 0.0 && index < -0.9 && abs(number/10.0) < 0.1 && abs(number) > 0.1,
                  in[0][3], result);
    result = cond(number < 0.0 && index > -1.1 && index < -0.9 && abs(number) < 0.1,
                  in[0][3], result);
  }

  if (showgrid) {
    result = result || (dot(fillcast<7>(ShConstAttrib1f(0.1)), in[0]) > 0.0) * 0.2;
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction
  } SH_END;

  ShAttrib1f SH_DECL(value);
  value.range(-500.1, 50.1);

  ShColor3f SH_DECL(background) = ShColor3f(0.69, 0.75, 0.68);
  ShColor3f SH_DECL(foreground) = ShColor3f(0.18, 0.20, 0.18);

  ShVector2f SH_DECL(offset) = ShVector2f(0.0, 0.25);
  ShVector2f SH_DECL(scale) = ShVector2f(1.0, 1.0);
  scale.range(0.01, 3.0);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);

    tc *= scale;
    tc(1) = 1.0 - tc(1);
    result = cond(lcd(tc - offset, value), foreground, background);
  } SH_END;
  return true;
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    list.push_back(new LCD(globals));
    return list;
  }
}
#else
static StaticLinkedShader<LCD> instance = 
       StaticLinkedShader<LCD>();
#endif


