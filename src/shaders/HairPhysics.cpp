#include <iostream>
#include <fstream>
#include <list>
#include <math.h>
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#include "Shader.hpp"
#include "Globals.hpp"
#include "Text.hpp"
#include "config.h"
#include <sh/ShObjMesh.hpp>

using namespace SH;
using namespace ShUtil;


struct hair
{
	float size;
	float root[3];
	float tangent[3];
	float surface[3];
};

class Hair : public Shader {
public:
  Hair();
  ~Hair();

  bool init();

  void bind();
  
  void render();
  
  ShProgram vertex() { return vsh_hair;}
  ShProgram fragment() { return fsh_hair;}
  
  ShProgram vsh_hair, vsh_head;
  ShProgram fsh_hair, fsh_head;

private:
	bool render_head;
	std::list<hair> hairs;
	
  static Hair* instance;
};

Hair::Hair()
  : Shader("Hair: Hair with physics")
{
}

Hair::~Hair()
{
}

void Hair::bind()
{
}

void Hair::render()
{
	if(render_head) // render a sphere for the head
	{
		shBind(vsh_head);
		shBind(fsh_head);
		for(int i=0 ; i<10 ; i++) {
			glBegin(GL_QUAD_STRIP);
			for(int j=0 ; j<10 ; j++) {
				float x = sin(i*M_PI/5)*cos(j*M_PI/5);
				float y = cos(i*M_PI/5);
				float z = sin(i*M_PI/5)*sin(j*M_PI/5);
				glNormal3f(x,y,z);
				glVertex3f(x,y,z);
				i++;
				x = sin(i*M_PI/5)*cos(j*M_PI/5);
				y = cos(i*M_PI/5);
				z = sin(i*M_PI/5)*sin(j*M_PI/5);
				i--;
				glNormal3f(x,y,z);
				glVertex3f(x,y,z);
			}
			glEnd();
		}
	}

	shBind(vsh_hair);
	shBind(fsh_hair);
	// render the hair
	for(std::list<hair>::iterator i = hairs.begin() ; i!=hairs.end() ; i++) {
		glBegin(GL_LINE_STRIP);
		glNormal3f(0.0,0.0,1.0);
		for(float y=0 ; y<i->size ; y=y+0.1) { // the length of the hair
			glMultiTexCoord3fARB(GL_TEXTURE0, i->tangent[0], i->tangent[1], i->tangent[2]);
			glMultiTexCoord4fARB(GL_TEXTURE0+1, y, i->surface[0], i->surface[1], i->surface[2]);
			glVertex3f(i->root[0],i->root[1],i->root[2]);
		}
		glEnd();
	}	
}

ShAttrib1f N(ShAttrib1f eta, ShAttrib1f cosGammai, ShAttrib1f sigmaa)
{
		ShAttrib1f cosGammat = sqrt(1+(cosGammai*cosGammai-1)/(eta*eta));
		ShAttrib1f cos2Gammat = 2*cosGammat*cosGammat - 1.0;
		ShAttrib1f Ntrt = (eta-1.0)/(eta+1.0);
		Ntrt *= Ntrt;
		Ntrt = Ntrt + (1.0-Ntrt)*pow(1.0-cosGammai,5.0) / pow(M_E,2.0*sigmaa(0)*(1.0+cos2Gammat));
		Ntrt *= Ntrt;
		ShAttrib1f F = (1.0/eta-1.0)/(1.0/eta+1.0);
		F *= F;
		Ntrt *= F + (1.0-F)*pow(1.0-cosGammat,5.0);
		return Ntrt;
}

bool Hair::init()
{
  //ShEnvironment::optimizationLevel = 0;
  std::cerr << "Initializing " << name() << std::endl;

	// rendering parameter
	render_head = false;
	bool render_first_highlight = true;
	bool render_sec_highlight = true;
	bool render_diffuse = true;

	// creation of the hair parameters
	srand(time(NULL));
	for(int i=0 ; i<50 ; i++) {
		for(int j=0 ; j<50 ; j++) {
			float x = sin(j*M_PI/200)*cos(i*M_PI/25);
			float y = cos(j*M_PI/200);
			float z = sin(j*M_PI/200)*sin(i*M_PI/25);
			hair newHair;
			newHair.size = (int)(3.0*rand()/(RAND_MAX+1.0));
			newHair.root[0] = x;
			newHair.root[1] = y;
			newHair.root[2] = z;
			newHair.surface[0] = x*0.5*(float)(rand())/(RAND_MAX+1.0);
			newHair.surface[1] = y*0.5*(float)(rand())/(RAND_MAX+1.0);
			newHair.surface[2] = z*0.5*(float)(rand())/(RAND_MAX+1.0);
			newHair.tangent[0] = -newHair.surface[2];
			newHair.tangent[1] = 0.0;
			newHair.tangent[2] = newHair.surface[0];
			hairs.push_back(newHair);
		}
	}
	
	ShVector3f SH_DECL(gravity) = ShVector3f(0.0,-0.5,0.0);
	gravity.range(-1.0,1.0);

	ShVector3f SH_DECL(windForce) = ShVector3f(0.0,0.0,0.0);
	windForce.range(-1.0,1.0);
	ShAttrib1f SH_DECL(windTime) = ShAttrib1f(0.0);	
	windTime.range(0.0,1.0);
	
	ShPoint3f SH_DECL(center) = ShPoint3f(0.0,0.0,0.0);
	center.range(-3.0,3.0);
	center.name("head position");
	ShAttrib1f SH_DECL(radius) = ShAttrib1f(1.0);
	radius.range(0.1,3.0);
	radius.name("head size");
	
  vsh_hair = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition3f ipos;
    ShInputNormal3f inorm;
		ShInputVector3f itan;
		ShInputAttrib4f tV; // time + initial direction of the hair
		ShVector3f V = tV(1,2,3);
		ShAttrib1f t = tV(0); 
		
    ShOutputPosition3f opos; // Position in NDC
    ShOutputNormal3f onorm;
		ShOutputVector3f otan;
		ShOutputVector3f osurf;
    ShOutputVector3f lightv; // direction to light
		ShOutputVector3f eyev;
		ShOutputVector3f halfv;

		// add simple physics
		gravity += windForce * sin(M_PI*windTime); // add the effect of the wind
		ipos += 0.5*gravity*t*t + V*t; // compute the position relative to the length
		ShVector3f isurf = V + gravity*t; // surface vector = velocity
		inorm = cross(itan,isurf); // compute the normal according to the velocity
		
		// collision with the face
		ShVector3f PosC = ipos-center;
		ShVector3f N = normalize(PosC);
		ShAttrib1f inside = sqrt(PosC|PosC)<radius;
		ipos = inside * (center+radius*N) + (1.0-inside)*ipos; // put the hair on the surface of the sphere when there is a collision
		
		opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
		otan = Globals::mv | itan; // Compute view-space tangent
		osurf = Globals::mv | isurf;
		
		ShPoint3f posv = (Globals::mv | ipos); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

		ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;

	ShAttrib1f SH_DECL(scale) = ShAttrib1f(100);
	scale.range(1.0,500.0);
	
  ShColor3f SH_DECL(color) = ShColor3f(0.74,0.47,0.47);
	color.name("diffuse color");

  ShColor3f SH_DECL(skinColor) = ShColor3f(0.92,0.76,0.7);
	skinColor.name("skin color");

	ShAttrib3f SH_DECL(sigmaa) = ShAttrib3f(0.74,0.84,0.9);
	sigmaa.name("absorption");
	sigmaa.range(0.0,1.0);
	
  fsh_hair = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f normal;
		ShInputVector3f tangent;
		ShInputVector3f surface;
    ShInputVector3f light;
		ShInputVector3f eye;
		ShInputVector3f half;
		
    ShOutputColor3f result;
		
		tangent = normalize(tangent);
    light = normalize(light);
		eye = normalize(eye);
		half = normalize(half);
		normal = normalize(normal);
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
		sinThetah = 0.5*sinThetah*rcp(cosThetah);
	
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
		ShAttrib1f eta1 = sqrt(eta*eta - 1.0 + cosThetad2) * rcp(cosThetad);
		ShAttrib1f Nr = (eta1-1.0)*rcp(eta1+1.0);
		Nr *= Nr;
		Nr = Nr + (1.0-Nr)*pow(1.0-cosGammai,5.0);
		Nr *= abs(cosGammai) * 0.25;
		
		// Compute Mtrt
		ShAttrib1f Mtrt = cosThetah*0.996917334 - sinThetah*0.078459096; // add 4.5 degrees
		Mtrt = pow(Mtrt,24);

		// Compute Ntrt
		ShAttrib1f a = 1.0;
		ShAttrib1f c = asin(rcp(eta1));
		ShAttrib1f etastar1 = 2.0*(eta-1.0)*a*a - eta + 2.0;
		ShAttrib1f etastar2 = 2.0*(eta-1.0)*rcp(a*a) - eta + 2.0;
		ShAttrib1f etastar = 0.5*(etastar1+etastar2 + cosThetah*(etastar1-etastar2));
		eta1 = sqrt(etastar*etastar - 1.0 + cosThetad2) * rcp(cosThetad);

		// solve a*x^3 + b*x^2 + c*x + d = 0
		ShAttrib1f M_PI3 = M_PI*M_PI*M_PI;
		ShAttrib1f phi = acos(cosPhi);
		ShAttrib1f Q = (M_PI*M_PI * (M_PI*rcp(8.0*c) - 0.75)) * 0.3333333;
		ShAttrib1f R = (M_PI3 * (M_PI*rcp(8.0*c) - phi*rcp(16.0*c))) * 0.5;
		ShAttrib1f D = Q*Q*Q + R*R;
		ShAttrib1f S = cond(R+sqrt(D), pow(max(0.0,R+sqrt(D)),1.0/3.0), ShAttrib1f(0.0));
		ShAttrib1f T = cond(R-sqrt(D), pow(max(0.0,R-sqrt(D)),1.0/3.0), ShAttrib1f(0.0));
		ShAttrib1f gammai = S+T;

		ShAttrib1f angle = cond(-D, acos(R*rcp(sqrt(-Q*Q*Q))), ShAttrib1f(0.0));
		gammai = cond(-D, 2.0*sqrt(-Q)*cos(angle/3.0), gammai); // test if 3 solutions
		cosGammai = cos(gammai);
		ShAttrib1f dPhidh = abs(cosGammai * rcp((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai*rcp(M_PI3)));
		ShAttrib3f Ntrt;
		Ntrt(0) = N(eta1, cosGammai, sigmaa(0));
		Ntrt(1) = N(eta1, cosGammai, sigmaa(1));
		Ntrt(2) = N(eta1, cosGammai, sigmaa(2));

		gammai = 2.0*sqrt(-Q)*cos((angle+2.0*M_PI)/3.0);
		cosGammai = cos(gammai);
		dPhidh = abs(cosGammai * rcp((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai*rcp(M_PI3)));
		ShAttrib3f Ntrt2;
		Ntrt2(0) = N(eta1, cosGammai, sigmaa(0));
		Ntrt2(1) = N(eta1, cosGammai, sigmaa(1));
		Ntrt2(2) = N(eta1, cosGammai, sigmaa(2));
		gammai = 2.0*sqrt(-Q)*cos((angle+4.0*M_PI)/3.0);
		cosGammai = cos(gammai);
		dPhidh = abs(cosGammai * rcp((24.0*c/M_PI-4.0) - 96.0*c*gammai*gammai*rcp(M_PI3)));
		Ntrt2(0) += N(eta1, cosGammai, sigmaa(0));
		Ntrt2(1) += N(eta1, cosGammai, sigmaa(1));
		Ntrt2(2) += N(eta1, cosGammai, sigmaa(2));
		Ntrt = cond(-D, Ntrt+Ntrt2, Ntrt); // test if 3 solutions
	
		if(render_first_highlight) {
			ShColor3f white = ShColor3f(1.0,1.0,1.0);
			result = white * 10 * Mr*Nr / cosThetad2;
		}
		else {
			result = ShColor3f(0.0,0.0,0.0);
		}
		if(render_sec_highlight) {
			result += Mtrt*Ntrt / cosThetad2;
		}
		result *= ((light|eye) > 0.0);
		
		if(render_diffuse) {
			result = (result + color*0.2) * pos(light|normal);
		}
	} SH_END;

	// the vertex shader used for the face
	vsh_head = SH_BEGIN_PROGRAM("gpu:vertex") {
		ShInputPosition3f ipos;
		ShInputNormal3f inorm;

		ShOutputPosition3f opos;
		ShOutputNormal3f onorm;
    ShOutputVector3f lightv;

		// change the sphere to fit with the parameters used on the hair vertex shader
		ipos *= 0.95*radius;
		ipos += center;
		
		opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
		ShPoint3f posv = (Globals::mv | ipos); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
	} SH_END;
	
 fsh_head = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f normal;
		ShInputVector3f light;
		ShOutputColor3f result = skinColor * pos(light|normal); // diffuse light
 } SH_END;
		
  return true;
}

Hair* Hair::instance = new Hair();
