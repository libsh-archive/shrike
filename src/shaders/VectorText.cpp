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
#include "ShFont.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"
#define PACK
#define NEIGHBOURS 9

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

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
segdist_t (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 point 1)
    ShPoint2f  x   //< test point
) {
    // compute 2D vector from first endpoint to x 
    ShAttrib2f v = x - L(0,1);
    // compute tangent  
    ShAttrib2f d = L(2,3) - L(0,1);
    // compute squared length of tangent
    ShAttrib1f d2 = (d|d);
    // compute t value of closest point on line
    ShAttrib1f t = (v|d)/d2;
    // clamp to range [0,1]
    t = pos(t);
    t = sat(t);
    // compute point on line 
    ShAttrib2f p = L(0,1) + t*d;

    // configure return value
    ShAttrib4f r;
    // compute vector from p to x (is gradient)
    r(2,3) = x - p;
    // compute squared distance
    r(0) = (r(2,3)|r(2,3));
    // compute sign using plane equation; normal is (-d(1),d(0))
    // r(1) = v(1)*d(0) - v(0)*d(1); (works fine for sign, bad pseudodistance)
    r(1) = (v(1)*d(0) - v(0)*d(1))*rsqrt(d2);

    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 point 1)
    int N,           // number of line segments
    ShPoint2f  x    //< test point
) {
    ShAttrib4f r = segdist_t(L[0],x);
    ShAttrib4f nr;
    for (int i=1; i<N; i++) {
       nr = segdist_t(L[i],x);
       r = cond(nr(0) < r(0),nr,r);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign 
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient
    r(2,3) = r(2,3)/r(0);
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

VectorText::~VectorText()
{
}

bool VectorText::init()
{
#ifdef NOPACK
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShFont font;
  font.loadFont("/home/zqin/vectortexture/freetype/p.txt");

  int width = font.width();
  int height = font.height();
  int edges = font.edges();
  float halfx = font.halfx();
  float halfy = font.halfy();
  int elements = 4;

  // textures for line segment endpoints
  ShUnclamped< ShArrayRect<ShAttrib4f> > ftexture[edges];
  for(int i=0; i<edges; i++) {
	  ftexture[i].size(width, height);
    ftexture[i].memory(font.memory(i));
  }

  // texture for number of edges
  // not used for now, just for debugging
  ShArray2D<ShAttrib1f> findex(width, height);
  findex.memory(font.edge());

  // may not need this... but just in case
  shUpdate();

  //debug info
  for(int i=0; i<height; i++) {
	  for(int j=0; j<width; j++) {
		  int num = (int)font.edgenum()[i*width+j];
		  std::cerr << (i * width + j) << " " << num << " " << std::endl;
		  for(int l=0; l<edges; l++) {
			  for(int m=0; m<elements; m++) {
				  int index = (i * width + j) * elements + m;
				  std::cout << font.coords(l)[index] << " ";
			  }
			  std::cout << std::endl;
		  }
	  }
  }

  std::cerr << " the image width is " << font.width() << std::endl;
  std::cerr << " the image height is " << font.height() << std::endl;
  std::cerr << " the image maxedge is " << font.edges() << std::endl;
  std::cerr << " the image halfx is " << font.halfx() << std::endl;
  std::cerr << " the image halfy is " << font.halfy() << std::endl;
#endif

#ifdef PACK
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShFont font;
  font.loadFont("/home/zqin/vectortexture/freetype/p.txt");

  int width = font.width();
  int height = font.height();
  int elements = 4;

  // textures for line segment endpoints
  ShUnclamped< ShArrayRect<ShAttrib4f> > ftexture(width, height);
  ftexture.memory(font.memory(0));

  //debug info
  for(int i=0; i<height; i++) {
	  for(int j=0; j<width; j++) {
		  std::cout << i << " " << j << std::endl;
		  for(int m=0; m<elements; m++) {
			  int index = (i * width + j) * elements + m;
			  std::cout << font.coords(0)[index] << " ";
		  }
		  std::cout << std::endl;
	  }
  }

  std::cerr << " the image width is " << font.width() << std::endl;
  std::cerr << " the image height is " << font.height() << std::endl;

  ShAttrib2f temp = ShAttrib2f(0.03125, 0.03125);
  ShAttrib2f size[9];
  size[0] = ShAttrib2f(-1.0/width, -1.0/height) - temp;
  size[1] = ShAttrib2f(0,          -1.0/height) - temp;
  size[2] = ShAttrib2f(1.0/width,  -1.0/height) - temp;
  size[3] = ShAttrib2f(-1.0/width,  0) - temp;
  size[4] = ShAttrib2f(0,           0) - temp;
  size[5] = ShAttrib2f(1.0/width,   0) - temp;
  size[6] = ShAttrib2f(-1.0/width,  1.0/height) - temp;
  size[7] = ShAttrib2f(0,           1.0/height) - temp;
  size[8] = ShAttrib2f(1.0/width,   1.0/height) - temp;
#endif

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
#ifdef NOPACK
    //ShAttrib2f x = tc * m_size - m_offset;
    ShAttrib2f x = tc - m_offset;
    ShAttrib4f L[edges];

    for(int i=0; i<edges; i++) {
    	L[i] = ftexture[i](x); 
    }
  
    ShAttrib4f r = segdists(L,edges,x);
#endif

#ifdef PACK
    ShAttrib2f x = tc;
    ShAttrib4f L[NEIGHBOURS];

    for(int i=0; i<NEIGHBOURS; i++) {
		x = tc + size[i];
    	L[i] = ftexture(x); 
    }
  
    ShAttrib4f r = segdists(L,NEIGHBOURS,x);
#endif

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
        // aliased rendering;
        o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
      } break;
      case 3: {
        // isotropically antialiased outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;;
        ShAttrib2f p;
        p(0) = smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 4: {
        // anisotropically antialiased outline rendering;
        ShAttrib2f fw;;
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
        p(0) = smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = smoothstep(-w,w,-r(1)-m_thres(1));;
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

bool VectorText::m_done_init = false;
ShAttrib1f VectorText::m_scale = ShAttrib1f(512);
ShVector2f VectorText::m_offset = ShVector2f(0.000976562,0.000976562);
ShAttrib1f VectorText::m_size = ShAttrib1f(512);
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

