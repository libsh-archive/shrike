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
#include <iostream>
#include <cstdarg>
#include "Shader.hpp"
#include "Globals.hpp"
#include "LCDSmall.hpp"

// you don't really want to be reading this
// see LCD.hpp for the nice readable version
// 
// Ultimately, the optimizer should do a few things
// that should make LCD.cpp a bit smaller,
// but at this point, this hand-optimized version
// is here to make LCD work on ATI cards

using namespace SH;


LCDSmall::LCDSmall()
  : Shader("LCD: Hand-Optimized")
{
}

LCDSmall::~LCDSmall()
{
}

template<int N>
ShAttrib<N, SH_CONST> mkconst(double offset, double a, ...)
{
  va_list ap;
  va_start(ap, a);
  float args[N];

  args[0] = a + offset;
  for (int i = 1; i < N; i++) {
    args[i] = va_arg(ap, double) + offset;
  }
  
  va_end(ap);
  
  return ShAttrib<N, SH_CONST>(args);
}

ShAttrib1f lcdSmall(const ShTexCoord2f& tc, ShAttrib1f number,
               int intDigits, int fracDigits, bool showgrid, bool handleneg,
               float w, float h, float t)
{
  float invwt = 1.0 / (w + t);
  float eps = 0.001;

  /* Represents range where segments are on
   * We have LT+LB only because it fits the 4-tuples well
   * (and helped out by a few instructions + # of params on ATI)
   *                   LT  LB  RT  RB  TT  CE  BB  LT+LB
   */
  ShAttrib<8, SH_CONST> segRange[2] = {
    mkconst<8>(-eps,  4.0,  2.0,  1.0,  1.0,  2.0,  2.0,  2.0,  6.0),
    mkconst<8>(eps,   5.0,  2.0,  4.0,  1.0,  3.0,  6.0,  6.0,  8.0)};

  ShAttrib<8, SH_CONST> segEnd =
    mkconst<8>(-eps,  9.0, 99.0,  7.0,  3.0,  5.0,  8.0,  8.0, 99.0);

  /* the final condition for segments is:
   * digit < 1 || (segRange[0] < digit < segRange[1]) || segEnd < digit
   * BUT remove the following thre special cases:
   * digit = 0, seg = CE 
   * digit = 4, seg = BB
   * digit = 7, seg = LT+LB
   */
  ShAttrib<4, SH_CONST> specialDigitRange[2] = {
    mkconst<4>(-eps, 99.0, 0.0, 4.0, 7.0),
    mkconst<4>(eps, 99.0, 0.0, 4.0, 7.0)}; 

// TODO remove 99 it's just so that long tuple swizzling doesn't occur
  
    /*
  ShAttrib<8, SH_CONST> posns[4] = {
    mkconst<8>(0.0, 0.0  , 0.0  , w - t, w - t , 0.0  , t          , 0.0  ,   0.0),  // left
    mkconst<8>(0.0, t    , t    , w    , w     , w    , w - t      , w    ,     t  ),  // right
    */

  // above does lots of repeated comparisons with the same numbers for the x-coords
  // Do only necessary comparisons and swizzle the results
  ShConstAttrib3f xRange[2] = {
    mkconst<3>(0.0, 0.0, 0.0, w-t),
    mkconst<3>(0.0,   t,   w,   w)};

  ShAttrib<8, SH_CONST> yRange[2] = {
    mkconst<8>(0.0, h/2.0, 0.0  , h/2.0, 0.0   , h - t, (h - t)/2.0, 0.0  ,   0.0),  // bottom
    mkconst<8>(0.0, h    , h/2.0, h    , h/2.0 , h    , (h + t)/2.0, t    ,     h)}; // top

  ShConstAttrib2f TEN_INT(10.0f, intDigits); // TODO remove this hack for packing constants
  
  ShAttrib1f result;

  ShTexCoord2f loc = tc;
  ShAttrib1f f = floor(loc(0) * invwt);
  loc(0) = mad(-f, w+t, loc(0));

  ShAttrib1f index = f - /*intDigits*/ TEN_INT(1);
  ShAttrib1f digitExtractor = pow(TEN_INT(0), index);
  ShAttrib1f digit = floor(frac(number * digitExtractor) * TEN_INT(0));
 

  if (handleneg) {
    digit = cond(number < 0.0, 9.0 - digit, digit);
  }

  // digit = abs(index); // Useful for debugging
  
  
  // check x ranges 
  ShAttrib3f xIn; // left, center, right
  xIn = (xRange[0] < loc(0)) && (loc(0) < xRange[1]);
  //            LT LB RT RB TT CE BB LT+LB
  int xSwiz[] = {0, 0, 2, 2, 1, 1, 1, 0};
  ShAttrib<8, SH_TEMP> in = xIn.swiz<8>(xSwiz);


  // now check y ranges 
  in *= (yRange[0] < loc(1)) && (loc(1) < yRange[1]);

  ShAttrib<8, SH_TEMP> segs;

  // get segment in/out bits based on digit
  segs = fillcast<8>(digit < eps); // digit < 0
  segs += (segRange[0] < digit) && (digit < segRange[1]);
  segs += segEnd < digit;

  // handle special cases
  ShAttrib4f special = (specialDigitRange[0] < digit) && (digit < specialDigitRange[1]);
  segs(4, 5, 6,7) -= special;

  result = in | segs; 

  // cut off extra fractional digits 
  result *= (index < (fracDigits - 1 + eps)); 
  
  if (handleneg) {
    result = cond(index < -1.9 && abs(number * 0.1f) < 0.1,
                  ShConstAttrib1f(0.0f), result);
    result = cond(number < 0.0 && index < -1.9 && abs(number * 0.1f) < 0.1 && abs(number) > 0.1,
                  in[3], result);
    result = cond(number < 0.0 && index > -2.1 && index < -1.9 && abs(number) < 0.1,
                  in[3], result);
  }

  if (showgrid) {
    result = result || (dot(fillcast<8>(ShConstAttrib1f(0.1)), in) > 0.0) * 0.2;
  }
  
  return result;
}

bool LCDSmall::init()
{
  ShVector2f SH_DECL(scale) = ShVector2f(1.0, 1.0);
  scale.range(0.01, 3.0);

  ShVector2f SH_DECL(offset) = ShVector2f(0.0, 0.25);

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
    tc(1) = 1.0 - tc(1);
    tc *= scale;
    tc -= offset;
  } SH_END;

  ShAttrib1f SH_DECL(value);
  value.range(0.0, 50.1);

  ShColor3f SH_DECL(background) = ShColor3f(0.69, 0.75, 0.68);
  ShColor3f SH_DECL(foreground) = ShColor3f(0.18, 0.20, 0.18);


  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    result = cond(lcdSmall(tc, value, 3, 0, false, false), foreground, background);
  } SH_END;
  return true;
}

LCDSmall LCDSmall::instance = LCDSmall();


