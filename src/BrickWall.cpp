#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <math.h>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

/* hyperbolic tangent */
ShAttrib1f tanh( ShAttrib1f x) {
	return (pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1);
}
	


class BrickID : public Shader {
public:
  BrickID();
  ~BrickID();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static BrickID instance;
};

BrickID::BrickID()
  : Shader("Tiling: BrickID")
{
}

BrickID::~BrickID()
{
}

bool BrickID::init()
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

  ShColor3f SH_DECL(brick) = ShColor3f(0.7, 0.1, 0.1);
  ShColor3f SH_DECL(mortar) = ShColor3f(0.3, 0.3, 0.3);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(3.0, 5.0);
  scale.name("brick size");
  scale.range(1.0, 100.0);

  ShAttrib2f SH_DECL(mortarsize) = ShAttrib2f(0.03, 0.03);
  mortarsize.name("mortar size");
  mortarsize.range(0.0, 0.1);

  ShAttrib1f SH_DECL(offset) = ShAttrib1f(0.5);
  offset.range(0.0, 1.0);
 
  ShAttrib3f SH_DECL(colorVariations) = ShAttrib3f(0.2, 0.1, 0.1);
  colorVariations.range(0.0, 1.0);
 
  ShAttrib1f NewBrick = 0.0;

  ShProgram brickID = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f pos;
    ShInputTexCoord2f tc;
    ShInputNormal3f norm;

    ShOutputAttrib1f id;
    ShOutputAttrib1f tmp;
    ShOutputNormal3f bumpnorm;

    tc *= scale;
    tmp = abs(floor(tc(1)+2)) * abs(floor(tc(0)+1));
    tc[0] = tc(0) - floor(tc(1))*offset; // change the horizontal position of a line
    tc[1] = tc(1) - floor(tc(1)+0.5);
    tc[0] = tc(0) - floor(tc(0)+0.5);

    bumpnorm = norm;
    //bumpnorm = bumpnorm + ShVector3f(tc(0)/abs(tc(0)) * 0.5 * (tanh(10*tc(0)+2)+1), ShAttrib1f(0.0), ShAttrib1f(0.0));
    bumpnorm = bumpnorm + ShVector3f(ShAttrib1f(0.0),tc(1)/abs(tc(1)) * 0.5 * (tanh(10*tc(1)+2)+1), ShAttrib1f(0.0));
    bumpnorm = cond( max(abs(tc(0)) > 0.5-1*mortarsize(0), abs(tc(1)) < 1*mortarsize(1)), bumpnorm, norm);
    bumpnorm = normalize(bumpnorm);
    
    ShAttrib1f inside;
    
    ShAttrib1f BrickColor;
    inside = min(abs(tc(0)) < 0.5-mortarsize(0), abs(tc(1)) > mortarsize(1));
    id = cond(inside, ShConstAttrib1f(1.0), ShConstAttrib1f(0.0));

  } SH_END;

  ShProgram brickModifier = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputAttrib1f tmp;
    ShOutputColor3f brickVariations;  
    for(int i=0 ; i<3 ; i++) {
      brickVariations[i] = colorVariations(i) * cellnoise<1>(tmp, false, false);
    }
    brickVariations += brick;
    
  } SH_END;
  
  ShProgram select = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputAttrib1f id;
    ShInputColor3f brickVariations;
    ShInputNormal3f normal;
    ShInputVector3f half;
    ShInputVector3f light;
 
    ShOutputColor3f result;
  
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);

    result= cond(id<0.5, mortar, brickVariations);

    ShAttrib1f irrad = pos(normal | light);
    result = result * irrad + result * pow(pos(normal | half), ShAttrib1f(30.0)) / (normal | light);
         
  } SH_END;
 
  fsh = select << ((((keep<ShAttrib1f>() & brickModifier & keep<ShNormal3f>()) << brickID)) & keep<ShVector3f>() & keep<ShVector3f>());
  
  return true;
}

BrickID BrickID::instance = BrickID();
