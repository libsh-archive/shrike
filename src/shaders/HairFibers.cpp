#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

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
  : Shader("Hair Fibers")
{
}

HairFiber::~HairFiber()
{
}

// The normalized Gaussian funciton is used to approximate the effects of the scales
ShAttrib1f gaussian(
	ShAttrib1f beta,
  ShAttrib1f x) {
	ShAttrib1f M = 1.0/sqrt(2*M_PI)*pow(M_E,-x*x/(2*beta*beta));
	return M;
}

// compute the fresnel coefficient with an angle as input
ShAttrib1f fresnel(
	ShAttrib1f eta,
	ShAttrib1f angle
) {
	ShAttrib1f s = (eta-1.0)/(eta+1.0);
	s *= s;
	return s + (1.0-s)*pow(1.0-cos(angle),5.0);
}

/* create a smoothstep between min and max
 * returns 0 if x<min
 * returns 1 if x>max
 * smooth between min and max
 */
 ShAttrib1f smoothstep(
	ShAttrib1f min,
	ShAttrib1f max,
	ShAttrib1f x
) {
	return cond(x<min, ShAttrib1f(0.0), cond(x>max, ShAttrib1f(1.0), (x-min)/(max-min)));
}

ShAttrib1f etaprime(
	ShAttrib1f eta,
	ShAttrib1f angle
) {
	return sqrt(eta*eta - sin(angle)*sin(angle)) / cos(angle);
}

ShAttrib1f N2(
	ShAttrib1f c,
	ShAttrib1f eta,
	ShColor3f sigmaa,
	ShAttrib1f gammai
) {
	ShAttrib1f gammat = 3*c*gammai/M_PI - 4*c*gammai*gammai*gammai/(M_PI*M_PI*M_PI);
	ShAttrib1f N = (1.0-fresnel(eta,gammai)) * pow(M_E,-2.0*sigmaa(0)*(1.0+cos(2.0*gammat)));
	N *= N * fresnel(1.0/eta,gammat) * abs(cos(gammai) / (2.0 * ((12.0*c/M_PI-2.0) - 48.0*c*gammai*gammai/(M_PI*M_PI*M_PI))));
	return N;
}

bool HairFiber::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
		ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light
		ShOutputVector3f otan;
		ShOutputVector3f eyev;
		
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
		otan = Globals::mv | itan; // Conpute view-space tangent

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

		ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
  } SH_END;
	
	ShAttrib1f SH_DECL(a) = ShAttrib1f(1.0);
	a.range(0.85,1.18);
	
  ShColor3f SH_DECL(color) = ShColor3f(0.74, 0.47, 0.47);
	
	ShColor3f SH_DECL(sigmaa) = ShColor3f(0.44,0.64,0.9); // attenuation in the fiber
	
	ShAttrib2f SH_DECL(scale) = ShAttrib2f(500.0,500);
	scale.range(1.0,1000.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputPosition4f posh;
		ShInputVector3f tangent;
		ShInputVector3f eye;
		
    ShOutputColor3f result;
    normal = normalize(normal);
		tangent = normalize(tangent);
    light = normalize(light);
		eye = normalize(eye);

		ShVector3f surface = cross(normal, tangent);
		surface = normalize(surface);

		normal(0) += 0.4*cellnoise<1>(scale(0)*tc(0), false);
		normal(1) += 0.4*cellnoise<1>(scale(1)*tc(0), false);
		normal = normalize(normal);

		ShVector3f azimut = (0.7*cellnoise<1>(100*tc(0), false)+0.3) * normal;
		azimut = normalize(azimut);

		ShAttrib1f thetai = M_PI/2 - acos(light | surface);
		ShAttrib1f thetar = M_PI/2 - acos(eye | -surface);
		ShAttrib1f thetah = (thetai + thetar) / 2.0;
		ShAttrib1f thetad = (thetar - thetai) / 2.0;

		ShAttrib1f phii = acos(light | azimut);
		ShAttrib1f phir = -acos(eye | azimut);
		ShAttrib1f phi = phir - phii;
   
		ShAttrib1f Mr = gaussian(8.0,thetah*180.0/M_PI-3.0f);  // angles are in degrees
		ShAttrib1f Mtt = gaussian(6.0,thetah*180.0/M_PI+1.5f);  
  	ShAttrib1f Mtrt = gaussian(15.0,thetah*180.0/M_PI+4.5f);
		
		ShAttrib1f eta = 1.55f;
		ShAttrib1f eta1 = etaprime(eta, thetad);

		// Conpute Nr
		ShAttrib1f Nr = fresnel(eta1, -phi/2); // -phi/2 is the root of the equation phi(0,h)-phi=0
		Nr *= abs(cos(phi/2)) / 4.0;

		ShAttrib1f eta2 = etaprime(eta, phii);
		ShAttrib1f c = asin(1.0f/eta2);
/*
 		// Compute Ntt
		ShAttrib1f Q = (M_PI*M_PI * (M_PI/(4.0*c) - 0.75))/3;
		ShAttrib1f R = -(M_PI*M_PI*M_PI * (phi/(8.0*c) - M_PI/(8.0*c)))/2;
		ShAttrib1f D = Q*Q*Q + R*R;
		ShAttrib1f gammai = pow(R + sqrt(D), 1.0f/3.0f) + pow(R - sqrt(D), 1.0f/3.0f);
		ShAttrib1f gammat = 3*c*gammai/M_PI - 4*c*gammai*gammai*gammai/(M_PI*M_PI*M_PI);
		
		ShAttrib1f Ntt = fresnel(eta1, gammai);
		Ntt = (1.0f - Ntt) * (1.0f - Ntt) * pow(M_E,-2.0*sigmaa(0)*(1.0f+cos(2.0f*gammat)));
		Ntt = - Ntt * abs(cos(h) /(2.0f * ((6.0f*c/M_PI-2.0f) - 24.0f*c*h*h/(M_PI*M_PI*M_PI))));
*/
		ShAttrib1f etastar1 = 2.0*(eta-1.0)*a*a - eta + 2.0;
		ShAttrib1f etastar2 = 2.0*(eta-1.0)/(a*a) - eta + 2.0;
		ShAttrib1f etastar = 0.5*(etastar1+etastar2 + cos(phii+phir)*(etastar1 - etastar2));
		eta1 = etaprime(etastar, thetad);

		eta2 = etaprime(eta1, thetai);
		ShAttrib1f hc = sqrt((4.0-eta2*eta2)/3.0);
		ShAttrib1f phic = cond(eta2 < 2.0, (12.0*c/M_PI - 2.0)*asin(hc) - 16.0*c*asin(hc)*asin(hc)*asin(hc)/(M_PI*M_PI*M_PI) + 2*M_PI, ShAttrib1f(0.0));
		ShAttrib1f deltah = min(0.5f, 2.0*sqrt(2.0*1.5*2.0/abs(96.0*c*hc/(M_PI*M_PI*M_PI) * asin(hc) / pow(1.0-asin(hc)*asin(hc),1.5))));
		deltah = cond(eta2 < 2.0, deltah, ShAttrib1f(0.5));
		ShAttrib1f t = cond(eta2 < 2.0, ShAttrib1f(1.0), smoothstep(2.0,2.3,eta2));

		ShAttrib1f Q = (M_PI*M_PI * (M_PI/(8.0*c) - 0.75))/3;
		ShAttrib1f R = -(M_PI*M_PI*M_PI * (phi/(16.0*c) - M_PI/(8.0*c))) / 2;
		ShAttrib1f D = Q*Q*Q + R*R;
		ShAttrib1f gammai = pow(R + sqrt(Q*Q*Q+R*R), 1.0f/3.0f) + pow(R - sqrt(Q*Q*Q+R*R), 1.0f/3.0f);
		ShAttrib1f angle = acos(R/sqrt(-Q*Q*Q));
		gammai = cond(D < 0.0, 2.0*sqrt(-Q)*cos(angle/3.0), gammai);
		ShAttrib1f Ntrt = N2(c, eta1, sigmaa, gammai);

		gammai = 2.0*sqrt(-Q)*cos((angle+2.0*M_PI)/3.0);
		ShAttrib1f Ntrt2 = N2(c, eta1, sigmaa, gammai);
		
		gammai = 2.0*sqrt(-Q)*cos((angle+4.0*M_PI)/3.0);
		Ntrt2 += N2(c, eta1, sigmaa, gammai);

		Ntrt = cond(D < 0.0, Ntrt+Ntrt2, Ntrt);
/*		
		Ntrt *= (1.0 - t*gaussian((phi-phic)*180.0/M_PI,1.5) / gaussian(0.0,1.5));
		Ntrt *= (1.0 - t*gaussian((phi+phic)*180.0/M_PI,1.5) / gaussian(0.0,1.5));
		Ntrt += t*0.4*deltah*(gaussian((phi-phic)*180.0/M_PI,1.5)+gaussian((phi+phic)*180.0/M_PI,1.5));
*/
		result = color * (Nr*Mr) / (cos(thetad)*cos(thetad));

	} SH_END;
  return true;
}

HairFiber HairFiber::instance = HairFiber();
