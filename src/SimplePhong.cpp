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
  : Shader("Phong: Simple")
{
}

SimplePhong::~SimplePhong()
{
}

bool SimplePhong::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f SH_DECL(ipos);
    ShInputNormal3f SH_DECL(inorm);
    
    ShOutputPosition4f SH_DECL(opos); // Position in NDC
    ShOutputNormal3f SH_DECL(onorm);
    ShInOutTexCoord2f SH_DECL(tc); // pass through tex coords
    ShOutputVector3f SH_DECL(halfv);
    ShOutputVector3f SH_DECL(lightv); // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f SH_DECL(posv) = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f SH_DECL(viewv) = -normalize(posv); // Compute view vector
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;
  
  ShColor3f SH_DECL(specular) = ShColor3f(0.5, 1.0, 1.0);
  ShColor3f SH_DECL(diffuse) = ShColor3f(1.0, 0.0, 0.0);
  ShAttrib1f SH_DECL(exponent) = ShAttrib1f(35.0);
  exponent.range(5.0f, 500.0f);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f SH_DECL(normal);
    ShInputTexCoord2f SH_DECL(tc); // ignore texcoords
    ShInputVector3f SH_DECL(half);
    ShInputVector3f SH_DECL(light);
    ShInputPosition4f SH_DECL(posh);

    ShOutputColor3f SH_DECL(result);
    
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    // Compute phong lighting.
    ShAttrib1f SH_DECL(irrad) = pos(normal | light);
    result = diffuse * irrad + specular * pow(pos(normal | half), exponent);
  } SH_END;
  return true;
}

SimplePhong SimplePhong::instance = SimplePhong();


