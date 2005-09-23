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
  static ShAttrib1f m_phongexp;
  static ShAttrib3f m_th;

  static ShAttrib2f m_thres;
  static ShColor3f m_color1, m_color2;
  static ShColor3f m_vcolor1, m_vcolor2;
};

VectorDoc::VectorDoc(int mode)
  : Shader(std::string("Vector Graphics: Vector Document") + 
	  ((mode == 0) ? ": Isotropically Antialiased" : 
	  ((mode == 1) ? ": Phong" : 
	  ((mode == 2) ? ": Anisotropically Antialiased" : 
	  ((mode == 3) ? ": Aliased" : 
	  ((mode == 4) ? ": Isotropically Antialiased Outline" : 
	  ((mode == 5) ? ": Anisotropically Antialiased Outline" : 
	  ((mode == 6) ? ": Gradient" : 
	  ((mode == 7) ? ": Filter Width" : 
	  ((mode == 8) ? ": Pseudodistance Isotropically Antialiased Outline" : 
	  ((mode == 9) ? ": Pseudodistance Anisotropically Antialiased Outline" : 
	  ((mode == 10) ? ": Biased Signed Distance" : 
	  ((mode == 11) ? ": Greyscale Biased Signed Distance" : 
	  ((mode == 12) ? ": Signed Distance" : 
	  ((mode == 13) ? ": Biased Signed Pseudodistance" : 
	  ((mode == 14) ? ": Greyscale Biased Signed Pseudodistance" : 
	                  ": Pseudodistance")))))))))))))))),
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
    m_size.range(0.0, 10.0);

    m_thres.name("threshold");
    m_thres.range(-1.0, 1.0);

    m_color1.name("color1");
    m_color2.name("color2");
    m_vcolor1.name("vcolor1");
    m_vcolor2.name("vcolor2");

    m_th.name("wedge");
    m_th.range(0.0001, 0.01);

    // m_phongexp.name("phongexp");
    // m_phongexp.range(5.0, 500.0);

    m_done_init = true;
  }
}

VectorDoc::~VectorDoc()
{
}

bool VectorDoc::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  /*
  // vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  // vsh = shSwizzle("texcoord", "posh") << vsh;

  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  vsh = shSwizzle("normal", "halfVec", "lightVec", "posh", "texcoord") << vsh;

  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  ShImage image;
  // image.loadPng(normalize_path(SHMEDIA_DIR "/textures/rustkd.png"));
  image.loadPng(normalize_path("/home/zqin/sh/shmedia/textures/paper.png"));
  ShTable2D<ShColor3fub> difftex(image.width(), image.height());
  difftex.memory(image.memory());
  // image.loadPng(normalize_path(SHMEDIA_DIR "/textures/rustks.png"));
  image.loadPng(normalize_path("/home/zqin/sh/shmedia/textures/paper.png"));
  ShTable2D<ShColor3fub> spectex(image.width(), image.height());
  spectex.memory(image.memory());
   
  fsh = ShKernelSurface::phong<ShColor3f>();
  fsh = fsh << namedCombine(access(difftex), access(spectex));
  // fsh = fsh << shExtract("kd") << specular;
  // fsh = fsh << shExtract("ks") << diffuse;
  fsh = fsh << shExtract("specExp") << exponent;
  fsh = fsh << shExtract("irrad") << lightColor;
  */


  // parameter 256: num of small grids
  // parameter 16;  num of small grids in one big grid
  // these parameter can generated automatically
  // according to the ratio of glyph height and width
  // TODO

  /*
  doc.initFont("font.txt", 64, 4);
  
  std::string str;
  
  str = "the princess bride";
  doc.string(str.length(), str.c_str(), 0.25, 4);
  */

  doc.initFont("font.txt", 16, 4);
  
  std::string str;

  // str = "How doth the little crocodile";
  // doc.string(str.length(), str.c_str(), 0.25, 17);

  /*  with texture
  str = "How doth the little crocodile";
  doc.string(str.length(), str.c_str(), 3, 20);

  str = "Improve his shining tail";
  doc.string(str.length(), str.c_str(), 3, 18.5);

  str = "And pour the waters of the Nile";
  doc.string(str.length(), str.c_str(), 3, 17);

  str = "On every golden scale";
  doc.string(str.length(), str.c_str(), 3, 15.5);

  str = "How cheerfully he seems to grin";
  doc.string(str.length(), str.c_str(), 3, 13.5);

  str = "How neatly spreads his claws";
  doc.string(str.length(), str.c_str(), 3, 12);

  str = "And welcomes little fishes in";
  doc.string(str.length(), str.c_str(), 3, 10.5);

  str = "With gently smiling jaws";
  doc.string(str.length(), str.c_str(), 3, 9);
  */

  /*
  // without texture
  str = "How doth the little crocodile";
  doc.string(str.length(), str.c_str(), 1, 15);

  str = "Improve his shining tail";
  doc.string(str.length(), str.c_str(), 1, 13.5);

  str = "And pour the waters of the Nile";
  doc.string(str.length(), str.c_str(), 1, 12);

  str = "On every golden scale";
  doc.string(str.length(), str.c_str(), 1, 10.5);

  str = "How cheerfully he seems to grin";
  doc.string(str.length(), str.c_str(), 1, 8.5);

  str = "How neatly spreads his claws";
  doc.string(str.length(), str.c_str(), 1, 7);

  str = "And welcomes little fishes in";
  doc.string(str.length(), str.c_str(), 1, 5.5);

  str = "With gently smiling jaws";
  doc.string(str.length(), str.c_str(), 1, 4);
  */

  /*
  str = "Premature";
  doc.string(9, str.c_str(), 0.25, 6, sp);

  str = "optimization";
  doc.string(12, str.c_str(), 0.25, 4.5, sp);

  str = "is the root";
  doc.string(11, str.c_str(), 0.25, 3, sp);

  str = "of all evil";
  doc.string(11, str.c_str(), 0.25, 1.5, sp);

  str = "ABC";
  doc.string(str.length(), str.c_str(), 3, 2);
  
  int t = 68;
  doc.string(1, &t, 1.0f, 2.0f);
  */

  int gly[4];
  for(int i=0; i<4; i++) 
  	//gly[i] = 19977;
  	gly[i] = 33457;
  doc.string(1, gly, 2.0f, 2.0f);
  doc.string(1, gly, 2.0f, 3.0f);
  doc.string(1, gly, 2.0f, 4.0f);
  doc.string(1, gly, 2.0f, 5.0f);

  /*
  str = "the armada";
  doc.string(str.length(), str.c_str(), 22, 15);

  str = "the farm";
  doc.string(str.length(), str.c_str(), 29, 16);

  str = "fishermen's";
  doc.string(str.length(), str.c_str(), 19.5, 21);

  str = "village";
  doc.string(str.length(), str.c_str(), 19.5, 20);

  str = "ambush";
  doc.string(str.length(), str.c_str(), 7, 20);

  str = "fire";
  doc.string(str.length(), str.c_str(), 0.25, 14);

  str = "swamp";
  doc.string(str.length(), str.c_str(), 0.25, 13);

  str = "cliffs of";
  doc.string(str.length(), str.c_str(), 8, 10.5);

  str = "insanity";
  doc.string(str.length(), str.c_str(), 8, 9.5);

  str = "lotharon's";
  doc.string(str.length(), str.c_str(), 23, 9);

  str = "castle";
  doc.string(str.length(), str.c_str(), 23, 8);

  str = "pursuit ship";
  doc.string(str.length(), str.c_str(), 14, 9.5);

  str = "zoo of";
  doc.string(str.length(), str.c_str(), 30, 5);

  str = "earth";
  doc.string(str.length(), str.c_str(), 30, 4);

  str = "the four white";
  doc.string(str.length(), str.c_str(), 10, 5);

  str = "horses";
  doc.string(str.length(), str.c_str(), 10, 4);
  */

  doc.stringEnd();


  vsh = SH_BEGIN_VERTEX_PROGRAM {
    ShInOutTexCoord2f u;
    ShInputNormal3f nm;
    ShInputPosition4f pm;

    ShOutputNormal3f nv;
    ShOutputVector3f hv;
    ShOutputVector3f lv;
    ShOutputPosition4f pd;

    ShOutputPoint3f pv = (Globals::mv|pm)(0,1,2);

    nv = normalize(Globals::mv | nm);
    
    ShVector3f vv = normalize(-pv);
    lv = normalize(Globals::lightPos - pv);
    hv = normalize(lv + vv);

    pd = (Globals::mvp | pm);

  } SH_END_PROGRAM;


  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShInputNormal3f nv;
    ShInputVector3f hv;
    ShInputVector3f lv;
    ShInputPosition4f pd;
    ShInputPoint3f pv;

    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = m_size*tc - m_offset;

    switch (m_mode) {
      case 0: {
        // isotropically antialiased rendering
	o = doc.isoAntialias(x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 1: {
        // phong
        o = doc.anisoAntialiasPhong(nv, hv, lv, pv, m_phongexp, m_th,
			x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 2: {
        // anisotropically antialiased rendering
        o = doc.anisoAntialias(x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 3: {
        // aliased rendering;
        o = doc.Alias(x, m_color1,m_color2, m_thres);
      } break;
      case 4: {
        // isotropically antialiased outline rendering
        o = doc.isoAntiOutline(x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 5: {
        // anisotropically antialiased outline rendering;
	o = doc.anisoAntiOutline(x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 6: {
        // gradient visualization
	o = doc.gradient(x, m_vcolor1, m_vcolor2);
      } break;
      case 7: {
        // filter width visualization
	o = doc.filterWidth(x);
      } break;
      case 8: {
        // isotropically antialiased pseudodistance outline rendering
	o = doc.isoAntiPseudoOutline(x, m_color1, m_color2, m_fw, m_thres);
      } break;
      case 9: {
        // anisotropically antialiased pseudodistance outline rendering
	o = doc.anisoAntiPseudoOutline(x, m_color1, m_color2, m_fw, m_thres);
      } break;;
      case 10: {
        // biased signed distance map visualization 
        o = doc.biasSignedDis1(x, m_scale, m_vcolor1, m_vcolor2);
      } break;
      case 11: {
        // biased signed distance map visualization 
	o = doc.biasSignedDis2(x, m_scale);
      } break;
      case 12: {
        // signed distance map visualization
	o = doc.signedDisMap(x, m_scale, m_vcolor2, m_vcolor2);
      } break;
      case 13: {
        // biased signed pseudodistance map visualization 
	o = doc.biasSignPseudoMap(x, m_scale, m_vcolor1, m_vcolor2);
      } break;
      case 14: {
        // biased signed pseudodistance map visualization 
	o = doc.biasSignPserdoMap(x, m_scale);
      } break;
      default: {
        // pseudodistance visualization
        o = doc.pseudoDis(x, m_scale, m_vcolor1, m_color2);
      } break;
    }
  } SH_END_PROGRAM;

  return true;
}

bool VectorDoc::m_done_init = false;
ShAttrib1f VectorDoc::m_scale = ShAttrib1f(1.0);
// ShVector2f VectorDoc::m_offset = ShVector2f(0,0);
ShVector2f VectorDoc::m_offset = ShVector2f(-1.01,-0.88);
ShAttrib1f VectorDoc::m_size = ShAttrib1f(1.0);
ShAttrib1f VectorDoc::m_fw = ShAttrib1f(1.0);
ShAttrib2f VectorDoc::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f VectorDoc::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f VectorDoc::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f VectorDoc::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f VectorDoc::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);
ShAttrib1f VectorDoc::m_phongexp = ShAttrib1f(10.0);
ShAttrib3f VectorDoc::m_th = ShAttrib3f(0.002, 0.002, 0.001);

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

