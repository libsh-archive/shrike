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
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class VectorText : public Shader {
public:
  VectorText(int mode);
  ~VectorText();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:

/* SegDist
 *
 * Compute distance and gradient from a point to a line segment.
 *
 * This functions returns the unsigned distance, an evaluation of the
 * plane equation (whose sign lets us know which side of the edge
 * we are on), and the gradient of the distance to a line segment.  
 * These are packed into the components of a 4-tuple.   
 *
 * We depend on dead-code elimination (which is supposed to work 
 * on components of tuples) to get rid of computations that are not 
 * needed.   
 */
ShAttrib4f //< true distance, signed value (dist from line), gradient vector
segdist (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 point 1)
    ShPoint2f  x   //< test point
) {
    // return value
    ShAttrib4f r;

    // compute 2D vectors from first endpoint to x (delta(0,1)) and
    // from second endpoint to x (delta(2,3)).
    ShAttrib4f delta = x(0,1,0,1) - L;

    // compute tangent d and normal d 
    ShAttrib4f dn = L(2,3,1,2) - L(0,1,3,0);
    // make normal unit length, store as result gradient
    r(2,3) = normalize(dn(2,3));

    // compute distance to line by projecting delta(0,1) onto normal
    r(1) = (r(2,3)|delta(0,1));  // signed distance from line
    // fix signs to get unsigned distance and its gradient
    r(0,2,3) = cond(r(1) > 0.0,r(1,2,3),-r(1,2,3)); 

    // compute dot products of d with delta vectors
    ShAttrib4f c = dn(0,1,0,1) * delta;
    c(0,1) = c(0,2) + c(1,3);  // c(0) = d|delta(0,1), c(1) = d|delta(2,3)

    // compute lengths of delta vectors
    ShAttrib4f s = delta * delta;  // square all components
    s(1,2) = s(0,2) + s(1,3);      // squared lengths

    // pick smaller lengths and vectors
    s(0) = sqrt(min(s(1),s(2))); // smaller length
    s(1,2) = cond(s(1) < s(2),delta(0,1),delta(2,3)); // appropriate gradient
    s(1,2) = s(1,2) * (ShAttrib1f(1.0)/s(0)); // normalize 

    // replace distances if beyond ends of line
    r(0,2,3) = cond(c(0) > 0.0,r(0,2,3),s(0,1,2));
    r(0,2,3) = cond(c(1) < 0.0,r(0,2,3),s(0,1,2));

    return r;
}

  ShProgram vsh, fsh;
  
  int m_mode;
  static bool m_done_init;
  static ShAttrib1f m_size;
  static ShAttrib1f m_scale;
  static ShVector2f m_offset;
  static ShAttrib1f m_fw;
  static ShAttrib2f m_thres;
  static ShColor3f m_color1, m_color2;
  static ShColor3f m_vcolor1, m_vcolor2;
};

VectorText::VectorText(int mode)
  : Shader(std::string("Vector Graphics: Vector Text") + 
	  ((mode == 0) ? ": Isotropically Antialiased" : 
	  ((mode == 1) ? ": Anisotropically Antialiased" : 
	  ((mode == 2) ? ": Aliased" : 
	  ((mode == 3) ? ": Isotropically Antialiased Outline" : 
	  ((mode == 4) ? ": Anisotropically Antialiased Outline" : 
	  ((mode == 5) ? ": Gradient" : 
	  ((mode == 6) ? ": Filter Width" : 
	  ((mode == 7) ? ": Pseudodistance Isotropically Antialiased Outline" : 
	  ((mode == 8) ? ": Pseudodistance Anisotropically Antialiased Outline" : 
	  ((mode == 9) ? ": Biased Signed Distance" : 
	  ((mode == 10) ? ": Greyscale Biased Signed Distance" : 
	  ((mode == 11) ? ": Signed Distance" : 
	  ((mode == 12) ? ": Biased Signed Pseudodistance" : 
	  ((mode == 13) ? ": Greyscale Biased Signed Pseudodistance" : 
	                  ": Pseudodistance"))))))))))))))),
    m_mode(mode)
{
  if (!m_done_init) {
    // Initialize parameters and static variables
    m_scale.name("scale");
    m_scale.range(0.0, 100.0);

    m_fw.name("filter width");
    m_fw.range(0.0, 10.0);

    m_offset.name("offset");
    m_offset.range(-10.0, 10.0);

    m_size.name("size");
    m_size.range(0.0, 100.0);

    m_thres.name("threshold");
    m_thres.range(-1.0, 1.0);

    m_color1.name("color1");
    m_color2.name("color2");
    m_vcolor1.name("vcolor1");
    m_vcolor2.name("vcolor2");

    m_done_init = true;
  }
}

VectorText::~VectorText()
{
}

bool VectorText::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  // a test consisting of two contours (a letter A, actually)
  const int N = 11;
  // const int N = 22;
  ShAttrib4f L[N];

  /* DOESN'T WORK! Ambiguity in sign at acute vertices 
   */
  L[0] = ShAttrib4f(0.3,0.0,0.0,0.0);
  L[1] = ShAttrib4f(0.0,0.0,0.5,1.5);
  L[2] = ShAttrib4f(0.5,1.5,0.9,1.5);
  L[3] = ShAttrib4f(0.9,1.5,1.4,0.0);
  L[4] = ShAttrib4f(1.4,0.0,1.1,0.0);
  L[5] = ShAttrib4f(1.1,0.0,1.0,0.3);
  L[6] = ShAttrib4f(1.0,0.3,0.4,0.3);
  L[7] = ShAttrib4f(0.4,0.3,0.3,0);
  L[8] = ShAttrib4f(0.5,0.6,0.9,0.6);
  L[9] = ShAttrib4f(0.9,0.6,0.7,1.2);
  L[10] = ShAttrib4f(0.7,1.2,0.5,0.6);
  /* */

  // THE FIX:
  // Can "break apart" acute vertices to resolve ambiguity.
  // Yes, this IS a really evil hack, but it's cheap computationally.
  // Still get good distance field, only slight rounding of corners,
  // since epsilon can be in last bit of precision...
  const float eps = 0.0001;
  // const float xoff = 1.8;
  // const int ioff = 11;
  // const float xoff = 0.0;
  // const int ioff = 0;
  // L[ioff+0] = ShAttrib4f(xoff+0.3,0.0-eps,xoff+0.0,0.0-eps);
  // L[ioff+1] = ShAttrib4f(xoff+0.0-eps,0.0+eps,xoff+0.5,1.5);
  // L[ioff+2] = ShAttrib4f(xoff+0.5,1.5,xoff+0.9,1.5);
  // L[ioff+3] = ShAttrib4f(xoff+0.9,1.5,xoff+1.4+eps,0.0+eps);
  // L[ioff+4] = ShAttrib4f(xoff+1.4,0.0-eps,xoff+1.1,0.0-eps);
  // L[ioff+5] = ShAttrib4f(xoff+1.1,0.0,xoff+1.0,0.3);
  // L[ioff+6] = ShAttrib4f(xoff+1.0,0.3,xoff+0.4,0.3);
  // L[ioff+7] = ShAttrib4f(xoff+0.4,0.3,xoff+0.3,0);
  // L[ioff+8] = ShAttrib4f(xoff+0.5,0.6-eps,xoff+0.9,0.6-eps);
  // L[ioff+9] = ShAttrib4f(xoff+0.9+eps,0.6,xoff+0.7+eps,1.2);
  // L[ioff+10] = ShAttrib4f(xoff+0.7-eps,1.2,xoff+0.5-eps,0.6);

  // a better fix: contract each line segment by epsilon.
  // this gives better angles for the pseudodistance (useful for
  // "sharp" miter rules).
  for (int i=0; i<N; i++) {
    ShVector2f d = normalize(L[i](2,3) - L[i](0,1));
    L[i](0,1) += d * eps;
    L[i](2,3) -= d * eps;
  }

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = (tc - m_offset) * m_size;

    // compute signed distance map
    ShAttrib4f r = segdist(L[0],x);
    ShAttrib4f nr;
    for (int i=1; i<N; i++) {
       nr = segdist(L[i],x);
       r = cond(nr(0) < r(0),nr,r);
    }
    // transfer sign
    r(0) = cond(r(1) < 0.0, -r(0), r(0));

    switch (m_mode) {
      case 0: {
        // isotropically antialiased rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib1f p = smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 1: {
        // anisotropically antialiased rendering
	ShAttrib2f fw;
	fw(0) = dx(x) | r(2,3);
	fw(1) = dy(x) | r(2,3);
	ShAttrib1f w = length(fw)*m_fw;
        ShAttrib1f p = smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 2: {
        // aliased rendering
        o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
      } break;
      case 3: {
        // isotropically antialiased outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib2f p;
	p(0) = smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 4: {
        // anisotropically antialiased outline rendering
	ShAttrib2f fw;
	fw(0) = dx(x) | r(2,3);
	fw(1) = dy(x) | r(2,3);
	ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
	p(0) = smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 5: {
        // gradient visualization
        o = 0.5 * (r(2) + 1.0) * m_vcolor1 
	  + 0.5 * (r(3) + 1.0) * m_vcolor2;
      } break;
      case 6: {
        // filter width visualization
	ShAttrib2f fw = fwidth(x);	      
	o = fw(0,1,0);
      } break;
      case 7: {
        // isotropically antialiased pseudodistance outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib2f p;
	p(0) = smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(1)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 8: {
        // anisotropically antialiased pseudodistance outline rendering
	ShAttrib2f fw;
	fw(0) = dx(x) | r(2,3);
	fw(1) = dy(x) | r(2,3);
	ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
	p(0) = smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(1)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 9: {
        // biased signed distance map visualization 
        o = (0.5 + r(0) * m_scale)(0,0,0) 
          * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 10: {
        // biased signed distance map visualization 
        o = (0.5 + r(0) * m_scale)(0,0,0);
      } break;
      case 11: {
        // signed distance map visualization
        o = (abs(r(0)) * m_scale)(0,0,0) 
          * cond(r(0) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 12: {
        // biased signed pseudodistance map visualization 
        o = (0.5 + r(1) * m_scale)(0,0,0) 
          * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
      case 13: {
        // biased signed pseudodistance map visualization 
        o = (0.5 + r(1) * m_scale)(0,0,0);
      } break;
      default: {
        // pseudodistance visualization
        o = (abs(r(1)) * m_scale)(0,0,0) 
          * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
      } break;
    }
  } SH_END_PROGRAM;
  return true;
}

bool VectorText::m_done_init = false;
ShAttrib1f VectorText::m_scale = ShAttrib1f(6.0);
ShVector2f VectorText::m_offset = ShVector2f(0.13,0.13);
ShAttrib1f VectorText::m_size = ShAttrib1f(2.0);
ShAttrib1f VectorText::m_fw = ShAttrib1f(1.0);
ShAttrib2f VectorText::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f VectorText::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorText::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorText::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f VectorText::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

VectorText vt_iaa = VectorText(0);
VectorText vt_aaa = VectorText(1);
VectorText vt_naa = VectorText(2);
VectorText vt_iaa_outline = VectorText(3);
VectorText vt_aaa_outline = VectorText(4);
VectorText vt_grad = VectorText(5);
VectorText vt_fw = VectorText(6);
VectorText vt_pd_iaa_outline = VectorText(7);
VectorText vt_pd_aaa_outline = VectorText(8);
VectorText vt_biased_distance = VectorText(9);
VectorText vt_grey_biased_distance = VectorText(10);
VectorText vt_distance = VectorText(11);
VectorText vt_biased_pdistance = VectorText(12);
VectorText vt_grey_biased_pdistance = VectorText(13);
VectorText vt_pdistance = VectorText(14);
