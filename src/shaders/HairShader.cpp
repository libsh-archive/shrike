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
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class HairFiber : public Shader {
public:
  HairFiber();
  ~HairFiber();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static HairFiber instance;
};

HairFiber::HairFiber()
  : Shader("Hair: Marschner Hair Fiber")
{
}

HairFiber::~HairFiber()
{
}

ShAttrib1f N2(ShAttrib1f eta, ShAttrib1f cosGammai, ShAttrib1f sigmaa)
{
		ShAttrib1f cosGammat = sqrt(1+(cosGammai*cosGammai-1)/(eta*eta));
		ShAttrib1f cos2Gammat = 2*cosGammat*cosGammat - 1.0;
		ShAttrib1f Ntrt = (eta-1.0)/(eta+1.0);
		Ntrt *= Ntrt;
		Ntrt = Ntrt + (1.0-Ntrt)*pow(1.0-cosGammai,5.0)  / (pow(M_E,2.0*sigmaa(0)*(1.0+cos2Gammat)));
		Ntrt *= Ntrt;
		ShAttrib1f F = (rcp(eta)-1.0)/(rcp(eta)+1.0);
		F *= F;
		Ntrt *= F + (1.0-F)*pow(1.0-cosGammat,5.0);
		return Ntrt;
}
				
bool HairFiber::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
		ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    ShOutputVector3f lightv; // direction to light
		ShOutputVector3f otan;
		ShOutputVector3f eyev;
		ShOutputVector3f halfv;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
		otan = Globals::mv | itan; // Compute view-space tangent
		
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

		ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    halfv = normalize(viewv + lightv); // Compute half vector

  } SH_END;

	ShAttrib1f SH_DECL(scale) = ShAttrib1f(100);
	scale.range(1.0,500.0);
	
  ShColor3f SH_DECL(color) = ShColor3f(0.74,0.47,0.47);

	ShAttrib1f SH_DECL(a) = ShAttrib1f(0.8);
	a.name("exentricity");
	a.range(0.5,1.0);

	ShAttrib3f SH_DECL(sigmaa) = ShAttrib3f(0.74,0.84,0.9);
	sigmaa.range(0.0,1.0);

	bool render_first_highlight = true;
	bool render_sec_highlight = true;
	bool render_diffuse = true;
	
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
    ShInputVector3f light;
		ShInputVector3f tangent;
		ShInputVector3f eye;
		ShInputVector3f half;
		
    ShOutputColor3f result;
		
		tangent = normalize(tangent);
    light = normalize(light);
		eye = normalize(eye);
		half = normalize(half);
		normal = normalize(normal);
		ShVector3f surface = cross(normal, tangent);
		surface = normalize(surface);

		ShAttrib1f eta = 1.55;
		
		ShAttrib1f sinThetai = light | surface;
		ShAttrib1f cosThetai = sqrt(1.0 - sinThetai*sinThetai);
		ShAttrib1f sinThetar = eye | -surface;
		ShAttrib1f cosThetar = sqrt(1.0 - sinThetar*sinThetar);
		
		ShAttrib1f cosThetad = cosThetar*cosThetai - sinThetar*sinThetai; // cos(thetar-thetai)
		cosThetad = sqrt((cosThetad + 1.0) * 0.5); // divide the angle by 2
		ShAttrib1f cosThetad2 = cosThetad*cosThetad;
		
		ShAttrib1f cosThetah = cosThetar*cosThetai + sinThetar*sinThetai; // cos(thetar+thetai)
		cosThetah = sqrt((cosThetah + 1.0) * 0.5); // divide by 2
		ShAttrib1f sinThetah = cosThetar*sinThetai + sinThetar*cosThetai;
		sinThetah = 0.5*sinThetah/cosThetah;
	
		ShVector2f lightproj = ShVector2f(light|tangent, light|normal);
		lightproj = normalize(lightproj);
		ShAttrib1f cosPhii = lightproj(1);
		ShAttrib1f sinPhii = lightproj(0);

		ShVector2f eyeproj = ShVector2f(eye|tangent, eye|normal);
		eyeproj = normalize(eyeproj);
		ShAttrib1f cosPhir = eyeproj(1);
		ShAttrib1f sinPhir = eyeproj(0);
		
		ShAttrib1f cosPhi = cosPhir*cosPhii + sinPhir*sinPhii;

		// Compute Mr
		ShAttrib1f Mr = cosThetah*0.998629535 + sinThetah*0.052335956; // remove 3 degrees
		Mr = pow(Mr,50);
		
		// Compute Nr
		ShAttrib1f cosGammai = sqrt((cosPhi + 1) * 0.5); // cos(phi/2)
		ShAttrib1f eta1 = sqrt(eta*eta - 1.0 + cosThetad2) / cosThetad;
		ShAttrib1f Nr = (eta1-1.0)/(eta1+1.0);
		Nr *= Nr;
		Nr = Nr + (1.0-Nr)*pow(1.0-cosGammai,5.0);
		Nr *= abs(cosGammai) * 0.25;
		
		// Compute Mtrt
		ShAttrib1f Mtrt = cosThetah*0.996917334 - sinThetah*0.078459096; // add 4.5 degrees
		Mtrt = pow(Mtrt,24);

		// Compute Ntrt
		ShAttrib1f c = asin(rcp(eta1));
		ShAttrib1f etastar1 = 2.0*(eta-1.0)*a*a - eta + 2.0;
		ShAttrib1f etastar2 = 2.0*(eta-1.0)/(a*a) - eta + 2.0;
		ShAttrib1f etastar = 0.5*(etastar1+etastar2 + cosThetah*(etastar1-etastar2));
		eta1 = sqrt(etastar*etastar - 1.0 + cosThetad2) / cosThetad;

		// solve a*x^3 + b*x^2 + c*x + d = 0
		ShAttrib1f M_PI3 = M_PI*M_PI*M_PI;
		ShAttrib1f phi = acos(cosPhi);
		ShAttrib1f Q = (M_PI*M_PI * (M_PI/(8.0*c) - 0.75)) * 0.3333333;
		ShAttrib1f R = (M_PI3 * (M_PI/(8.0*c) - phi/(16.0*c))) * 0.5;
		ShAttrib1f D = Q*Q*Q + R*R;
		ShAttrib1f S = cond(R+sqrt(D), pow(max(0.0,R+sqrt(D)),1.0/3.0), ShAttrib1f(0.0));
		ShAttrib1f T = cond(R-sqrt(D), pow(max(0.0,R-sqrt(D)),1.0/3.0), ShAttrib1f(0.0));
		ShAttrib1f gammai = S+T;

		ShAttrib1f angle = cond(-D, acos(R/(sqrt(-Q*Q*Q))), ShAttrib1f(0.0));
		gammai = cond(-D, 2.0*sqrt(-Q)*cos(angle/3.0), gammai); // test if 3 solutions
		cosGammai = cos(gammai);
		ShAttrib1f dPhidh = abs(cosGammai /((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai/M_PI3));
		ShAttrib3f Ntrt;
		Ntrt(0) = N2(eta1, cosGammai, sigmaa(0));
		Ntrt(1) = N2(eta1, cosGammai, sigmaa(1));
		Ntrt(2) = N2(eta1, cosGammai, sigmaa(2));

		gammai = 2.0*sqrt(-Q)*cos((angle+2.0*M_PI)*0.333333);
		cosGammai = cos(gammai);
		dPhidh = abs(cosGammai /((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai/M_PI3));
		ShAttrib3f Ntrt2;
		Ntrt2(0) = N2(eta1, cosGammai, sigmaa(0));
		Ntrt2(1) = N2(eta1, cosGammai, sigmaa(1));
		Ntrt2(2) = N2(eta1, cosGammai, sigmaa(2));
		gammai = 2.0*sqrt(-Q)*cos((angle+4.0*M_PI)*0.33333);
		cosGammai = cos(gammai);
		dPhidh = abs(cosGammai / ((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai/M_PI3));
		Ntrt2(0) += N2(eta1, cosGammai, sigmaa(0));
		Ntrt2(1) += N2(eta1, cosGammai, sigmaa(1));
		Ntrt2(2) += N2(eta1, cosGammai, sigmaa(2));
		Ntrt = cond(-D, Ntrt+Ntrt2, Ntrt); // test if 3 solutions
		
		if(render_first_highlight) {
			result = (Mr*Nr / cosThetad2)(0,0,0);
		}
		else {
			result = ShColor3f(0.0,0.0,0.0);
		}
		if(render_sec_highlight) {
			result += Mtrt*Ntrt  / cosThetad2;
		}
		if(render_diffuse) {
			result = (result + color*0.2) * pos(light|normal);
		}

	} SH_END;
  return true;
}

HairFiber HairFiber::instance = HairFiber();
