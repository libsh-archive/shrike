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
 * Compute distance from a point to a line segment.
 *
 * This functions returns the squared distance, the true
 * distance, and a value whose sign lets us know which side of the edge
 * we are on.  These are packed into the zeroth, first, and second 
 * components of a tuple.   We depend on dead-code elimination (which
 * hopefully will be extended to work on components of tuples) to get
 * rid of computations that are not needed.   
 */
ShAttrib3f //< squared distance, true distance, and signed value
segdist (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 point 1)
    ShPoint2f  x   //< test point
) {
    // compute 2D vectors from first endpoint to x (delta(0,1)) and
    // from second endpoint to x (delta(2,3)).
    ShAttrib4f delta = x(0,1,0,1) - L;

    // compute unit tangent and normal 
    ShAttrib4f dn = L(2,3,1,2) - L(0,1,3,0);
    ShAttrib3f n_len;
    n_len(0) = (dn(2,3)|dn(2,3));
    n_len(1) = sqrt(n_len(0));
    n_len(2) = ShAttrib1f(1.0)/n_len(1);
    dn(2,3) = n_len(2,2) * dn(2,3);  // unit normal, now

    ShAttrib3f r;  // return value

    // compute distance to line by projecting delta(0,1) onto normal
    r(2) = (dn(2,3)|delta(0,1));  // signed distance from line
    r(0) = r(2)*r(2);             // squared distance from line
    r(1) = abs(r(2));             // unsigned distance from line

    // compute dot products of d with delta vectors
    ShAttrib4f c;
    c = dn(0,1,0,1) * delta;
    c(0,1) = c(0,2) + c(1,3);

    // compute distance to endpoints
    delta = delta * delta;  // square all components
    delta(0,1) = delta(0,2) + delta(1,3);  // squared lengths
    delta(0) = min(delta(0),delta(1));     // minimum squared length
    delta(1) = sqrt(delta(0));             // unsigned length

    // replace distances if beyond ends of line
    r(0,1) = cond(c(0) > 0.0,r(0,1),delta(0,1));
    r(0,1) = cond(c(1) < 0.0,r(0,1),delta(0,1));

    return r;
}

  ShProgram vsh, fsh;
  
  int m_mode;
  static bool m_done_init;
  static ShAttrib1f m_size;
  static ShAttrib1f m_scale;
  static ShVector2f m_offset;
  static ShAttrib1f m_fw;
  static ShColor3f m_color1, m_color2;
  static ShColor3f m_vcolor1, m_vcolor2;
};

VectorText::VectorText(int mode)
  : Shader(std::string("Vector Graphics: Vector Text") + 
		  ((mode == 0) ? ": Antialiased" : 
		   ((mode == 1) ? ": Aliased" : ": Distance"))),
    m_mode(mode)
{
  if (!m_done_init) {
    // Initialize parameters and static variables
    m_scale.name("scale");
    m_scale.range(0.0, 5.0);

    m_fw.name("filter width");
    m_fw.range(0.0, 10.0);

    m_offset.name("offset");
    m_offset.range(-10.0, 10.0);

    m_size.name("size");
    m_size.range(0.0, 100.0);

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
  // const int N = 21;
  ShAttrib4f L[N];

  /* DOESN'T WORK! Ambiguity in sign at acute vertices.
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
  */

  // THE FIX:
  // Can "break apart" acute vertices to resolve ambiguity.
  // Yes, this IS a really evil hack, but it's cheap computationally.
  // Still get good distance field, only slight rounding of corners,
  // since epsilon can be in last bit of precision...
  const float eps = 0.0001;
  // const float xoff = 1.8;
  // const int ioff = 11;
  const float xoff = 0.0;
  const int ioff = 0;
  L[ioff+0] = ShAttrib4f(xoff+0.3,0.0-eps,xoff+0.0,0.0-eps);
  L[ioff+1] = ShAttrib4f(xoff+0.0-eps,0.0+eps,xoff+0.5,1.5);
  L[ioff+2] = ShAttrib4f(xoff+0.5,1.5,xoff+0.9,1.5);
  L[ioff+3] = ShAttrib4f(xoff+0.9,1.5,xoff+1.4+eps,0.0+eps);
  L[ioff+4] = ShAttrib4f(xoff+1.4,0.0-eps,xoff+1.1,0.0-eps);
  L[ioff+5] = ShAttrib4f(xoff+1.1,0.0,xoff+1.0,0.3);
  L[ioff+6] = ShAttrib4f(xoff+1.0,0.3,xoff+0.4,0.3);
  L[ioff+7] = ShAttrib4f(xoff+0.4,0.3,xoff+0.3,0);
  L[ioff+8] = ShAttrib4f(xoff+0.5,0.6-eps,xoff+0.9,0.6-eps);
  L[ioff+9] = ShAttrib4f(xoff+0.9+eps,0.6,xoff+0.7+eps,1.2);
  L[ioff+10] = ShAttrib4f(xoff+0.7-eps,1.2,xoff+0.5-eps,0.6);

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = (tc - m_offset) * m_size;

    // compute signed distance map
    ShAttrib3f r = segdist(L[0],x);
    ShAttrib3f nr;
    for (int i=1; i<N; i++) {
       nr = segdist(L[i],x);
       r = cond(nr(0) < r(0),nr,r);
    }
    r(0,1) = cond(r(2) < 0.0, -r(0,1), r(0,1));

    if (0 == m_mode) {
        // antialiased rendering
        ShAttrib2f fw = fwidth(x);
	fw(0) = max(fw(0),fw(1))*m_fw;
        ShAttrib1f p = smoothstep(-fw(0),fw(1),r(1));
        o = lerp(p,m_color2,m_color1);
    } else if (1 == m_mode) {
	// aliased rendering
        o = cond(r(2) >= 0.0,m_color2,m_color1);
    } else {
	// distance map visualization
        o = (abs(r(1)) * m_scale)(0,0,0) * cond(r(1) >= 0.0,m_vcolor2,m_vcolor1);
    }
  } SH_END_PROGRAM;
  return true;
}

bool VectorText::m_done_init = false;
ShAttrib1f VectorText::m_scale = ShAttrib1f(1.0);
ShVector2f VectorText::m_offset = ShVector2f(0.0,0.0);
ShAttrib1f VectorText::m_size = ShAttrib1f(1.0);
ShAttrib1f VectorText::m_fw = ShAttrib1f(1.0);
ShColor3f VectorText::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorText::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorText::m_vcolor1 = ShColor3f(1.0, 0.0, 0.0);
ShColor3f VectorText::m_vcolor2 = ShColor3f(0.0, 1.0, 0.0);

VectorText vt_with_aa = VectorText(0);
VectorText vt_without_aa = VectorText(1);
VectorText vt_distance = VectorText(2);

