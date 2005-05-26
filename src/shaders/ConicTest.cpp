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

#define N_ITER 1

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class ConicTest : public Shader {
public:
  ConicTest(int mode);
  ~ConicTest();

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

ConicTest::ConicTest(int mode)
  : Shader(std::string("Vector Graphics: Conic Test") + 
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

ConicTest::~ConicTest()
{
}

bool ConicTest::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  // data structure for precomputed conic info (use accessors so data can be packed)
  class Conic {
	private:
      ShAttrib4f a[3];  // data.   should probably use a 12-tuple and let system do packing.
	public:
	  // default constructor does nothing
	  Conic () {}
	  // set up with raw coefficients 
	  void init (
		const ShAttrib1f& alpha, 
		const ShAttrib1f& beta, 
		const ShVector2f& w,
		const ShPoint2f& p, 
		const ShVector2f& d, 
		const ShVector2f& v, 
		const ShAttrib1f& s,
		const ShAttrib1f& c  
      ) {
	    // store data
		a[0] = ShAttrib4f(alpha,beta,w(0),w(1));
		a[1] = ShAttrib4f(p(0),p(1),s,c);
		a[2] = ShAttrib4f(d(0),d(1),v(0),v(1));
	  }
	  // set up with raw coefficients 
	  void init (
		double alpha, double beta, // first row of rotation matrix
		double wx, double wy,      // translation vector
		double px, double py,      // constant coefficient 
		double dx, double dy,      // linear coefficient
		double vx, double vy,      // quadratic coefficient
		double s,                  // scale of parameter t wrt x in canonical coords
		double c                   // translation of t to center parabola about 0
      ) {
	    init (
		  ShAttrib1f(alpha), 
		  ShAttrib1f(beta), 
		  ShVector2f(wx,wy),
		  ShPoint2f(px,py), 
		  ShVector2f(dx,dy), 
		  ShVector2f(vx,vy), 
		  ShAttrib1f(s),
		  ShAttrib1f(c)  
        );
	  }
	  // set up with Bezier control points
	  void bezier (
	    const ShPoint2f& P0,
	    const ShPoint2f& P1,
	    const ShPoint2f& P2
      ) {
	    ShVector2f d = 2.0*(P1 - P0);
	    ShVector2f v = (P0 - P1) - (P1 - P2);
		ShAttrib1f k = 1.0/(v|v);
	    ShVector2f u = ShVector2f(v(1),-v(0));
	    ShVector2f w;  
		w(0) = - (k*(u|P0) - 0.5*k*k*(v|d)*(u|d));
		w(1) = - (k*(v|P0) - 0.25*k*k*(v|d)*(v|d));
	    ShAttrib1f s = k*(u|d); 
	    ShAttrib1f c = 0.5*k*(v|d); 
		init(k*u(0),k*u(1),w,P0,d,v,s,c);
	  }
	  // set up with Bezier control points
	  void bezier (
		double p0x, double p0y,    // first control point
		double p1x, double p1y,    // second control point
		double p2x, double p2y     // quadratic coefficient
      ) {
		ShPoint2f P0(p0x,p0y);
		ShPoint2f P1(p1x,p1y);
		ShPoint2f P2(p2x,p2y);
		bezier(P0,P1,P2);
	  }
	  // transform input point to canonical coordinates
	  ShAttrib2f canonical (const ShAttrib2f& x) {
	    ShAttrib2f xc;
		xc(0) =  a[0](0)*x(0) + a[0](1)*x(1) + a[0](2);
		xc(1) = -a[0](1)*x(0) + a[0](0)*x(1) + a[0](3);
		return xc;
      }
	  // evaluate quadratic (in original space)
	  ShAttrib2f position (const ShAttrib1f& t) {
	    ShAttrib2f Y = a[2](0,1) + a[2](2,3)*t;
		ShAttrib2f P = a[1](0,1) + Y*t;
		return P;
	  }
	  // evaluate tangent of quadratic (in original space)
	  ShAttrib2f tangent (const ShAttrib1f& t) {
		ShAttrib2f T = a[2](0,1) + 2.0*a[2](2,3)*t;
		return T;
	  }
	  // evaluate normal of quadratic (in original space)
	  ShAttrib2f normal (const ShAttrib1f& t) {
		// compute normal by rotating tangent
		ShAttrib2f N = a[2](1,0) + 2.0*a[2](3,2)*t;  // tangent, x/y switched
		N(1) = -N(1);  // negate y
		return N;
	  }
	  // approximate, using reguli-falsi, parameter value of closest point
	  ShAttrib1f solve(ShAttrib2f x) {
	    // get curve parameter
		ShAttrib1f s = a[1](2);

	    // generate interval guaranteed to have solution
		ShAttrib3f G;
		G(0,1) = ShAttrib2f(sign(x(0))*sqrt(pos(x(1)))/s,x(0)/s);

		// set up coefficients for distance cubic 
		ShAttrib3f cc;
		cc(0) = s*x(0);
		cc(1) = 2.0*x(1) - s*s;
		cc(2) = -2.0;  

		// evaluate distance cubic at endpoints of interval
        ShAttrib3f A;
		A(0,1) = cc(0,0) + (cc(1,1) + cc(2,2)*G(0,1)*G(0,1))*G(0,1);

	    // refine estimate using reguli-falsi
		for (int i=0; i<N_ITER; i++) {
		  G(2) = (A(0)*G(1) - A(1)*G(0))/(A(0) - A(1));
		  A(2) = cc(0) + (cc(1) + cc(2)*G(2)*G(2))*G(2);
		  ShAttrib1f c = (A(2)*A(0) > 0.0);
		  G(0,1) = cond(c,G(2,1),G(0,2));
		  A(0,1) = cond(c,A(2,1),A(0,2));
	    }
		G(2) = (A(0)*G(1) - A(1)*G(0))/(A(0) - A(1));

		// transform estimate back from canonical space to standard parameter space
		return G(2) - a[1](3);
	  }
	  // solve for clamped t value of closest point on curve segment
	  // compute squared distance, sign, unnormalized gradient vector to closest point on segment
	  ShAttrib4f dist (ShAttrib2f x) {
        // transform test point to canonical coordinate system
        ShAttrib2f xc = canonical(x);
		// find t value of closest point on canonical quadratic
		ShAttrib1f t = solve(xc);

		// clamp to bounds
		ShAttrib1f ct = pos(sat(t));
		// ShAttrib1f ct = t;
		// find point on curve, tangent, and normal 
		ShAttrib2f cP = position(ct);
		ShAttrib2f uP = position(t);
		ShAttrib2f N = normal(t);
		// compute distance, gradient, signed distance, etc.
        ShAttrib4f r; // square distance, sign (plane equation), unnorm gradient vector
		r(2,3) = cP - x;
		r(0) = r(2,3) | r(2,3);
		r(1) = (uP - x) | normalize(N);  // useful for pseudodistance

		// r(0,1) = cond(t>0.5,ShAttrib2f(t,t),ShAttrib2f(t,-t));

		// DEBUG
		// just evaluate sign of xc relative to parabola
		// ShAttrib1f s = a[1](2);
		// r(0) = xc(1) - (1.0/(s*s))*xc(0)*xc(0); 
		// r(1) = r(0);
		// r(0) *= r(0);
		return r;
	  }
  };

  struct BezierControl {
      ShAttrib2f P[3];
      BezierControl() {}
      void control (
          double p0x, double p0y,
          double p1x, double p1y,
          double p2x, double p2y
      ) {
	  P[0] = ShAttrib2f(p0x,p0y);
	  P[1] = ShAttrib2f(p1x,p1y);
	  P[2] = ShAttrib2f(p2x,p2y);
      }
      void contract (double eps = 0.0001) {
          ShVector2f d0 = normalize(P[1] - P[0]);
          P[0] += d0 * eps;
	      ShVector2f d1 = normalize(P[2] - P[1]);
          P[2] -= d1 * eps;
      }
  };

  // Data for a test character (a D)
  const int N = 6;
  BezierControl B[N];
  B[0].control(0.1,0.1, 0.2,0.5, 0.1,0.9);  
  B[1].control(0.1,0.9, 0.9,0.9, 0.9,0.5);  
  B[2].control(0.9,0.5, 0.9,0.1, 0.1,0.1);  
  B[3].control(0.3,0.2, 0.7,0.2, 0.7,0.5);  
  B[4].control(0.7,0.5, 0.7,0.8, 0.3,0.8);  
  B[5].control(0.3,0.8, 0.4,0.5, 0.3,0.2);  

  // Simple test data
  // const int N = 1;
  // BezierControl B[N];
  // B[0].control(0.1,0.5, 0.5,0.1, 0.9,0.5);  

  Conic C[N];
  for (int i=0; i<N; i++) {
      B[i].contract();
      C[i].bezier(B[i].P[0],B[i].P[1],B[i].P[2]);
  }

  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f tc;
    ShOutputColor3f o;

    // transform texture coords (should be in vertex shader really, but)
    ShAttrib2f x = tc * m_size - m_offset;

	// find closest conic
    ShAttrib4f r = C[0].dist(x);
    for (int i=1; i<N; i++) {
       ShAttrib4f nr = C[i].dist(x);
       r = cond(nr(0) < r(0),nr,r);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign 
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient
    r(2,3) = r(2,3)/r(0);

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
        // aliased rendering
        o = cond(r(0)+m_thres(0) > 0.0,m_color2,m_color1);
      } break;
      case 3: {
        // isotropically antialiased outline rendering
        ShAttrib2f fw = fwidth(x);
        ShAttrib1f w = max(fw(0),fw(1))*m_fw;
        ShAttrib2f p;
	p(0) = deprecated_smoothstep(-w,w,r(0)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(0)-m_thres(1));
        o = lerp((1-p(0))*(1-p(1)),m_color1,m_color2);
      } break;
      case 4: {
        // anisotropically antialiased outline rendering
	ShAttrib2f fw;
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
	p(0) = deprecated_smoothstep(-w,w,r(1)+m_thres(0));
        p(1) = deprecated_smoothstep(-w,w,-r(1)-m_thres(1));
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

bool ConicTest::m_done_init = false;
ShAttrib1f ConicTest::m_scale = ShAttrib1f(6.0);
ShVector2f ConicTest::m_offset = ShVector2f(0.0,0.0);
ShAttrib2f ConicTest::m_celloffset = ShAttrib2f(0.2,0.2);
ShAttrib2f ConicTest::m_cellperiod = ShAttrib2f(2.0,2.0);
ShAttrib1f ConicTest::m_size = ShAttrib1f(1.0);
ShAttrib1f ConicTest::m_fw = ShAttrib1f(1.0);
ShAttrib2f ConicTest::m_thres = ShAttrib2f(0.0,0.05);
ShColor3f ConicTest::m_color1 = ShColor3f(0.0, 0.0, 0.0);
ShColor3f ConicTest::m_color2 = ShColor3f(1.0, 1.0, 1.0);
ShColor3f ConicTest::m_vcolor1 = ShColor3f(1.0, 0.0, 1.0);
ShColor3f ConicTest::m_vcolor2 = ShColor3f(1.0, 1.0, 0.0);

ConicTest ctest_iaa = ConicTest(0);
ConicTest ctest_aaa = ConicTest(1);
ConicTest ctest_naa = ConicTest(2);
ConicTest ctest_iaa_outline = ConicTest(3);
ConicTest ctest_aaa_outline = ConicTest(4);
ConicTest ctest_grad = ConicTest(5);
ConicTest ctest_fw = ConicTest(6);
ConicTest ctest_pd_iaa_outline = ConicTest(7);
ConicTest ctest_pd_aaa_outline = ConicTest(8);
ConicTest ctest_biased_distance = ConicTest(9);
ConicTest ctest_grey_biased_distance = ConicTest(10);
ConicTest ctest_distance = ConicTest(11);
ConicTest ctest_biased_pdistance = ConicTest(12);
ConicTest ctest_grey_biased_pdistance = ConicTest(13);
ConicTest ctest_pdistance = ConicTest(14);

