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
#include "ShDoc.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "dist_util.hpp"

class VectorDoc : public Shader {
public:
  VectorDoc(int mode);
  ~VectorDoc();

  bool init();
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

private:

  ShDoc doc;
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

VectorDoc::VectorDoc(int mode)
  : Shader(std::string("Vector Graphics: Vector Document") + 
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
    m_scale.range(0.0, 30.0);

    m_fw.name("filter width");
    m_fw.range(0.0, 10.0);

    m_offset.name("offset");
    m_offset.range(-10.0, 10.0);

    m_size.name("size");
    m_size.range(0.0, 1000.0);

    m_thres.name("threshold");
    m_thres.range(-1.0, 1.0);

    m_color1.name("color1");
    m_color2.name("color2");
    m_vcolor1.name("vcolor1");
    m_vcolor2.name("vcolor2");

    m_done_init = true;
  }
}

VectorDoc::~VectorDoc()
{
}

bool VectorDoc::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  // parameter 256: num of small grids
  // parameter 16;  num of small grids in one big grid
  doc.initFont("font.txt", 256, 16);
  
  std::string str;

  str = "We Present a representation of";
  doc.string(32, str.c_str(), 0.25, 15);

  str = "font glyphs suitable for realtime";
  doc.string(33, str.c_str(), 0.25, 13.5);

  str = "scalable text rendering on GPUs";
  doc.string(32, str.c_str(), 0.25, 12);

  str = "Contours and sharp features can";
  doc.string(31, str.c_str(), 0.25, 10.5);

  str = "be exactly reconstructed using a";
  doc.string(32, str.c_str(), 0.25, 9);

  str = "constant amount of computation";
  doc.string(30, str.c_str(), 0.25, 7.5);

  str = "time per pixel. A combination of";
  doc.string(32, str.c_str(), 0.25, 6);

  str = "texture data and procedural com";
  doc.string(31, str.c_str(), 0.25, 4.5);

  str = "putation is used to recreate the";
  doc.string(32, str.c_str(), 0.25, 3);

  str = "signed distance field and its gra";
  doc.string(33, str.c_str(), 0.25, 1.5);

  str = "dient.";
  doc.string(6, str.c_str(), 0.25, 0);

  /*
  str = "Premature";
  doc.string(9, str.c_str(), 0.25, 6, sp);

  str = "optimization";
  doc.string(12, str.c_str(), 0.25, 4.5, sp);

  str = "is the root";
  doc.string(11, str.c_str(), 0.25, 3, sp);

  str = "of all evil";
  doc.string(11, str.c_str(), 0.25, 1.5, sp);
  */

  doc.stringEnd();

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = m_size*tc - m_offset;

    ShAttrib4f r = doc.shortestDis(x);

    switch (m_mode) {
      case 0: {
        // isotropically antialiased rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 1: {
        // anisotropically antialiased rendering
        ShAttrib2f fw;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib1f p = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        o = lerp(p,m_color2,m_color1);
      } break;
      case 2: {
        // aliased rendering;
        o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
      } break;
      case 3: {
        // isotropically antialiased outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 4: {
        // anisotropically antialiased outline rendering;
        ShAttrib2f fw;;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 5: {
        // gradient visualization
        o = 0.5 * (r(2) + 1.0) * m_vcolor1 
          + 0.5 * (r(3) + 1.0) * m_vcolor2;;
      } break;
      case 6: {
        // filter width visualization
        ShAttrib2f fw = fwidth(x);	      
        o = fw(0,1,0);
      } break;
      case 7: {
        // isotropically antialiased pseudodistance outline rendering
        ShAttrib2f fw = fwidth(x);;
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));;
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 8: {
        // anisotropically antialiased pseudodistance outline rendering
        ShAttrib2f fw;
        fw(0) = dx(x) | r(2,3);
        fw(1) = dy(x) | r(2,3);
        ShAttrib1f w = length(fw)*m_fw;
        ShAttrib2f p;
        p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;;
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

bool VectorDoc::m_done_init = false;
ShAttrib1f VectorDoc::m_scale = ShAttrib1f(1.0);
ShVector2f VectorDoc::m_offset = ShVector2f(0,0);
ShAttrib1f VectorDoc::m_size = ShAttrib1f(1.0);
ShAttrib1f VectorDoc::m_fw = ShAttrib1f(1.0);
ShAttrib2f VectorDoc::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f VectorDoc::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorDoc::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorDoc::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f VectorDoc::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

VectorDoc vd_iaa = VectorDoc(0);
VectorDoc vd_aaa = VectorDoc(1);
VectorDoc vd_naa = VectorDoc(2);
VectorDoc vd_iaa_outline = VectorDoc(3);
VectorDoc vd_aaa_outline = VectorDoc(4);
VectorDoc vd_grad = VectorDoc(5);
VectorDoc vd_fw = VectorDoc(6);
VectorDoc vd_pd_iaa_outline = VectorDoc(7);
VectorDoc vd_pd_aaa_outline = VectorDoc(8);
VectorDoc vd_biased_distance = VectorDoc(9);
VectorDoc vd_grey_biased_distance = VectorDoc(10);
VectorDoc vd_distance = VectorDoc(11);
VectorDoc vd_biased_pdistance = VectorDoc(12);
VectorDoc vd_grey_biased_pdistance = VectorDoc(13);
VectorDoc vd_pdistance = VectorDoc(14);

