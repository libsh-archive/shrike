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

#include "dist_util.hpp"

class VectorTest : public Shader {
public:
  VectorTest(int mode);
  ~VectorTest();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:

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
  static ShAttrib2f m_celloffset, m_cellperiod;
};

VectorTest::VectorTest(int mode)
  : Shader(std::string("Vector Graphics: Vector Test") + 
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

    m_celloffset.name("cell offset");
    m_celloffset.range(-1.0, 1.0);

    m_cellperiod.name("cell period");
    m_cellperiod.range(0.5, 10.0);

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

VectorTest::~VectorTest()
{
}

bool VectorTest::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  // a test consisting of two contours (a letter A, actually)
  const int N = 11;
  // const int N = 22;
  ShAttrib4f L[N];

  // Data for a simple "A" character
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

  // An Evil Hack: contract each line segment by epsilon.
  // This resolves the ambiguity over which line segment is "closer"
  // when a shared vertex is the closest point on a line segment, and
  // allows the correct sign to be determined based only on the closest
  // line segment.   By splitting the vertex by contracting both 
  // vertices by the same amount, this gives a plane separating the
  // two new vertices that is at the right angle for "sharp" miter rules
  // when using the pseudodistance for outline and "Bold" versions of
  // glyphs.
  const float eps = 0.0001;
  for (int i=0; i<N; i++) {
    ShVector2f d = normalize(L[i](2,3) - L[i](0,1));
    L[i](0,1) += d * eps;
    L[i](2,3) -= d * eps;
  }

  // Convert second point to a vector to that point
  for (int i=0; i<N; i++) {
    L[i](2,3) = L[i](2,3) - L[i](0,1);
  }

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = (tc - m_offset) * m_size;
    // x = (x - m_celloffset) % m_cellperiod; // tile plane 

    // compute signed distance map, sign field, and gradient of distance map
    ShAttrib4f r = segdists_da(L,N,x);

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

  std::cerr << "Finished initialising" << std::endl;
  return true;
}

bool VectorTest::m_done_init = false;
ShAttrib1f VectorTest::m_scale = ShAttrib1f(6.0);
ShVector2f VectorTest::m_offset = ShVector2f(0.13,0.13);
ShAttrib2f VectorTest::m_celloffset = ShAttrib2f(0.2,0.2);
ShAttrib2f VectorTest::m_cellperiod = ShAttrib2f(2.0,2.0);
ShAttrib1f VectorTest::m_size = ShAttrib1f(2.0);
ShAttrib1f VectorTest::m_fw = ShAttrib1f(1.0);
ShAttrib2f VectorTest::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f VectorTest::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorTest::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorTest::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f VectorTest::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

VectorTest vtest_iaa = VectorTest(0);
VectorTest vtest_aaa = VectorTest(1);
VectorTest vtest_naa = VectorTest(2);
VectorTest vtest_iaa_outline = VectorTest(3);
VectorTest vtest_aaa_outline = VectorTest(4);
VectorTest vtest_grad = VectorTest(5);
VectorTest vtest_fw = VectorTest(6);
VectorTest vtest_pd_iaa_outline = VectorTest(7);
VectorTest vtest_pd_aaa_outline = VectorTest(8);
VectorTest vtest_biased_distance = VectorTest(9);
VectorTest vtest_grey_biased_distance = VectorTest(10);
VectorTest vtest_distance = VectorTest(11);
VectorTest vtest_biased_pdistance = VectorTest(12);
VectorTest vtest_grey_biased_pdistance = VectorTest(13);
VectorTest vtest_pdistance = VectorTest(14);

