#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Lafortune : public Shader {
public:
  Lafortune();
  ~Lafortune();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Lafortune instance;
};

Lafortune::Lafortune()
  : Shader("Basic Lighting Models: Lafortune")
{
}

Lafortune::~Lafortune()
{
}

bool Lafortune::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
		ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
		ShOutputVector3f otan;
		ShOutputVector3f osurf;
    ShOutputVector3f lightv; // direction to light
		ShOutputVector3f viewv;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
		otan = Globals::mv | itan; // Compute view-space tangent
		osurf = cross(onorm,otan);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
    ShPoint3f eye = -normalize(posv); // view vector
    viewv = normalize(eye - posv);

  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
	
	ShAttrib3f SH_DECL(D) = ShAttrib3f(-1.0,1.0,1.0);
	D.range(-10.0,10.0);
	
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
		ShInputVector3f tangent;
		ShInputVector3f surface;
    ShInputVector3f light;
		ShInputVector3f view;

    ShOutputColor3f result;
    
    normal = normalize(normal);
		tangent = normalize(tangent);
		surface = normalize(surface);
    light = normalize(light);

		ShVector3f lS = ShVector3f(D(0)*light|tangent, D(1)*light|surface, D(2)*light|normal);
		ShVector3f vS = ShVector3f(view|tangent, view|surface, view|normal);
				
    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
		result = diffuse*irrad + specular*pow((lS|vS), exponent);
		
  } SH_END;
  return true;
}

Lafortune Lafortune::instance = Lafortune();

