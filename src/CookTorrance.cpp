#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"


using namespace SH;
using namespace ShUtil;

class CookTorrance : public Shader {
public:
  CookTorrance();
  ~CookTorrance();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static CookTorrance instance;
};

CookTorrance::CookTorrance()
  : Shader("Basic Lighting Models: Cook-Torrance")
{
}

CookTorrance::~CookTorrance()
{
}

bool CookTorrance::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f tangent;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f lightv; // direction to light
    ShOutputVector3f eyev; // direction to the eye
    ShOutputVector3f halfv; // half vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShMatrix3x3f mToTangent;
    mToTangent[0] = (Globals::mv | tangent)(0,1,2);
    mToTangent[1] = onorm;
    ShVector3f tang0 = (mToTangent | ShVector3f(1,0,0));
    ShVector3f tang1 = (mToTangent | ShVector3f(0,1,0));
    mToTangent[2] = cross(tang0,tang1); // ^ does not work ?

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
    //lightv = (mToTangent | lightv);

    ShPoint3f viewv = -normalize(posv); // view vector
    eyev = normalize(viewv - posv);
    //eyev = normalize(mToTangent | viewv); // Compute eye direction

    halfv = normalize(viewv + lightv); // Compute half vector
    
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(.2, 0.5, 0.9);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f light;
    ShInputVector3f eye;
    ShInputVector3f half;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    light = normalize(light);
    eye = normalize(eye);
    half = normalize(half);

    // Beckman's distribution function
    ShAttrib1f normalDotHalf = (normal | half);
    ShAttrib1f normalDotHalf2 = normalDotHalf * normalDotHalf;
    ShAttrib1f roughness2 = 0.15 * 0.15; // roughness fixed at 0.15
    ShAttrib1f exponent = -(1 - normalDotHalf2) /
	    (normalDotHalf2 * roughness2); // Compute the exponent value
    ShAttrib1f D = pow(M_E, exponent) /
	    (roughness2 * normalDotHalf2*normalDotHalf2); // Compute the distribution function

    // Fresnel term
    ShAttrib1f normalDotEye = (normal | eye);
    ShAttrib1f refindex = 0.15;
    ShAttrib1f F = refindex + (1 - refindex) * pow(1 - normalDotEye , 5); // Compute the Fresnel term

    // self shadowing term
    ShAttrib1f normalDotLight = (normal | light);
    ShAttrib1f X = 2.0 * normalDotHalf / (eye | half);
    ShAttrib1f G = min(1.0, min(X * normalDotLight, X * normalDotEye));

    ShAttrib1f CT = (D*F*G) / (normalDotEye * M_PI);
    
    ShAttrib3f specular = color * max(0.0, CT);
    ShAttrib3f diffuse = color * max(0.0, normalDotLight);
    result = diffuse + specular;
    
  } SH_END;
  return true;
}

CookTorrance CookTorrance::instance = CookTorrance();


