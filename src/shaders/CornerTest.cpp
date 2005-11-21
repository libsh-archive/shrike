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

class CornerTest : public Shader {
public:
  CornerTest(int mode);
  ~CornerTest();

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

CornerTest::CornerTest(int mode)
  : Shader(std::string("Vector Graphics: Corner Test") + 
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

CornerTest::~CornerTest()
{
}

bool CornerTest::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  // a test consisting of two contours (a letter A, actually)
  const int K = 11;
  ShAttrib4f c[K];
  ShAttrib4f d[K];

  // Data for a simple "A" character 
  // (normals are computed later, 0's for now)
  c[0] = ShAttrib4f(0.0,0.0, 0.0,0.0);
  d[0] = ShAttrib4f(0.15,0.0, 0.25,0.75);

  c[1] = ShAttrib4f(0.5,1.5, 0.0,0.0);
  d[1] = ShAttrib4f(-0.25,-0.75, 0.2,0.0);
  
  c[2] = ShAttrib4f(0.9,1.5, 0.0,0.0);
  d[2] = ShAttrib4f(-0.2,-0.0, 0.25,-0.75);
  
  c[3] = ShAttrib4f(1.4,0.0, 0.0,0.0);
  d[3] = ShAttrib4f(-0.25,0.75, -0.15,0.0);
  
  c[4] = ShAttrib4f(1.1,0.0, 0.0,0.0);
  d[4] = ShAttrib4f(0.15,0.0, -0.05,0.15);
  
  c[5] = ShAttrib4f(1.0,0.3, 0.0,0.0);
  d[5] = ShAttrib4f(0.05,-0.15, -0.3,0.0);
  
  c[6] = ShAttrib4f(0.4,0.3, 0.0,0.0);
  d[6] = ShAttrib4f(0.3,0.0, -0.05,-0.15);
  
  c[7] = ShAttrib4f(0.3,0.0, 0.0,0.0);
  d[7] = ShAttrib4f(0.05,0.15, -0.15,0.0);
  
  c[8] = ShAttrib4f(0.9,0.6, 0.0,0.0);
  d[8] = ShAttrib4f(-0.2,0.0, -0.1,0.3);
  
  c[9] = ShAttrib4f(0.7,1.2, 0.0,0.0);
  d[9] = ShAttrib4f(0.1,-0.3, -0.1,-0.3);
  
  c[10] = ShAttrib4f(0.5,0.6, 0.0,0.0);
  d[10] = ShAttrib4f(0.1,0.3, 0.2,0.0);

  // compute normals of separating planes
  for (int i=0; i<K; i++) {
      ShVector2f g[2];
      g[0] = normalize(d[i](0,1));
      g[1] = normalize(d[i](2,3));
      ShVector2f v = g[0] + g[1];
      c[i](2) = v(1);
      c[i](3) = -v(0);
      c[i](2,3) *= sign(c[i](2,3)|d[i](2,3));
  }

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = (tc - m_offset) * m_size;
    // x = (x - m_celloffset) % m_cellperiod; // tile plane 

    // compute signed distance map, sign field, and gradient of distance map
    ShAttrib4f r = cornerdists(c,d,K,x);

    switch (m_mode) {
      case 0: {
        // isotropically antialiased rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = SH::max(fw(0),fw(1))*m_fw;
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
        ShAttrib1f w = SH::max(fw(0),fw(1))*m_fw;
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
        ShAttrib1f w = SH::max(fw(0),fw(1))*m_fw;
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

bool CornerTest::m_done_init = false;
ShAttrib1f CornerTest::m_scale = ShAttrib1f(6.0);
ShVector2f CornerTest::m_offset = ShVector2f(0.13,0.13);
ShAttrib2f CornerTest::m_celloffset = ShAttrib2f(0.2,0.2);
ShAttrib2f CornerTest::m_cellperiod = ShAttrib2f(2.0,2.0);
ShAttrib1f CornerTest::m_size = ShAttrib1f(2.0);
ShAttrib1f CornerTest::m_fw = ShAttrib1f(1.0);
ShAttrib2f CornerTest::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f CornerTest::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f CornerTest::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f CornerTest::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f CornerTest::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

static CornerTest iaa = CornerTest(0);
static CornerTest aaa = CornerTest(1);
static CornerTest naa = CornerTest(2);
static CornerTest iaa_outline = CornerTest(3);
static CornerTest aaa_outline = CornerTest(4);
static CornerTest grad = CornerTest(5);
static CornerTest fw = CornerTest(6);
static CornerTest pd_iaa_outline = CornerTest(7);
static CornerTest pd_aaa_outline = CornerTest(8);
static CornerTest biased_distance = CornerTest(9);
static CornerTest grey_biased_distance = CornerTest(10);
static CornerTest distance = CornerTest(11);
static CornerTest biased_pdistance = CornerTest(12);
static CornerTest grey_biased_pdistance = CornerTest(13);
static CornerTest pdistance = CornerTest(14);

