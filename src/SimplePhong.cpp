#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class SimplePhong : public Shader {
public:
  SimplePhong();
  ~SimplePhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SimplePhong instance;
};

SimplePhong::SimplePhong()
  : Shader("Basic Lighting Models: Phong: Simple")
{
}

SimplePhong::~SimplePhong()
{
}

bool SimplePhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f view;
    ShOutputVector3f halfv;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f view;
    ShInputVector3f half;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    // Compute phong lighting.
    ShMatrix3x3f identity = ShMatrix3x3f();
    ShMatrix3x1f normalmat; // to transpose the normal
    normalmat[0] = sqrt(2) * normal[0]; // need to multiply the matrix by 2
    normalmat[1] = sqrt(2) * normal[1];
    normalmat[2] = sqrt(2) * normal[2];
    ShMatrix3x3f Householder = (normalmat | transpose(normalmat));
    Householder -= identity; // Compuse Householder transformation matrix
    ShMatrix1x3f viewtranspose; // to transpose the view vector
    viewtranspose[0][0] = view[0];
    viewtranspose[0][1] = view[1];
    viewtranspose[0][2] = view[2];
    ShAttrib1f irrad = pos(normal | light);
    result = diffuse * irrad + specular * pow(pos(viewtranspose | Householder | light), exponent) / (normal | light);
  } SH_END;
  return true;
}

SimplePhong SimplePhong::instance = SimplePhong();


// the modified Phong model
class ModifiedPhong : public Shader {
public:
  ModifiedPhong();
  ~ModifiedPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static ModifiedPhong instance;
};

ModifiedPhong::ModifiedPhong()
  : Shader("Basic Lighting Models: Phong: Modified")
{
}

ModifiedPhong::~ModifiedPhong()
{
}

bool ModifiedPhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f view;
    ShOutputVector3f halfv;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f view;
    ShInputVector3f half;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    ShMatrix3x3f identity = ShMatrix3x3f();
    ShMatrix3x1f normalmat; // to transpose the normal
    normalmat[0] = sqrt(2) * normal[0]; // need to multiply the matrix by 2
    normalmat[1] = sqrt(2) * normal[1];
    normalmat[2] = sqrt(2) * normal[2];
    ShMatrix3x3f Householder = (normalmat | transpose(normalmat));
    Householder -= identity; // Compuse Householder transformation matrix
    ShMatrix1x3f viewtranspose; // to transpose the view vector
    viewtranspose[0][0] = view[0];
    viewtranspose[0][1] = view[1];
    viewtranspose[0][2] = view[2];
    result = diffuse * irrad / M_PI  +
	    specular * (exponent+2)/(2*M_PI) * pow(pos(viewtranspose | Householder | light), exponent);
  } SH_END;
  return true;
}

ModifiedPhong ModifiedPhong::instance = ModifiedPhong();

// the Blinn-Phong model
class BlinnPhong : public Shader {
public:
  BlinnPhong();
  ~BlinnPhong();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static BlinnPhong instance;
};

BlinnPhong::BlinnPhong()
  : Shader("Basic Lighting Models: Phong: Blinn-Phong")
{
}

BlinnPhong::~BlinnPhong()
{
}

bool BlinnPhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f halfv;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords
    ShInputVector3f half;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;
    
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f irrad = pos(normal | light);
    result = diffuse * irrad + specular * pow(pos(normal | half), exponent) / (normal | light);
  } SH_END;
  return true;
}

BlinnPhong BlinnPhong::instance = BlinnPhong();


