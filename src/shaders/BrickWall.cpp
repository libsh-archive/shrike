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
  : Shader("Tiling: Brick Wall")
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
 
  /* Create the bricks and the mortar between them
   */
  ShProgram brickID = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f pos;
    ShInOutTexCoord2f tc;
    ShInOutNormal3f norm;
    ShOutputAttrib1f changeNorm;
    ShOutputAttrib1f id; // define if the current point belongs to a brick or to the mortar
    ShOutputAttrib1f changeColor; // used to generate different colors

    tc *= scale;
    changeNorm = tc(0)*tc(1)*pos(0)*pos(1);
    tc[0] = tc(0) - floor(tc(1))*offset; // change the horizontal position of a line
    changeColor = abs(floor(tc(1))) * abs(floor(tc(0)+0.5)); // change the color for each brick
    tc[1] = tc(1) - floor(tc(1)+0.5);
    tc[0] = tc(0) - floor(tc(0)+0.5);

    ShAttrib1f inside;
    inside = min(abs(tc(0)) < 0.5-mortarsize(0), abs(tc(1)) > mortarsize(1)); // limits of a brick
    id = cond(inside, ShConstAttrib1f(1.0), ShConstAttrib1f(0.0));

  } SH_END;

  
  /* Add bump-mapping to the wall
   * edges are bumpmapped with a tanh function
   * bricks and mortar are bumpmapped with some noise
   */
  ShProgram bumpmap = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc;
    ShInputNormal3f norm;
    ShInputAttrib1f changeNorm;
    ShOutputNormal3f bumpnorm = norm;
 
    ShVector3f normDeformation;

    for(int i=0 ; i<2 ; i++) {
      bumpnorm[i] += 0.03*cellnoise<1>(changeNorm, false, false);
    }

    // set the limits of edges
    ShAttrib1f verticalLimits = min(abs(tc(0)) < 0.5-mortarsize(0)+0.01, abs(tc(0)) > 0.5-mortarsize(0)-0.01);
    ShAttrib1f horizontalLimits = min(abs(tc(1)) > mortarsize(1)-0.01, abs(tc(1)) < mortarsize(1)+0.01);
    // change the normals with a tanh function
    normDeformation = ShVector3f(tc(0)/abs(tc(0)) * 0.5 * (tanh(10*abs(tc(0))-0.5-mortarsize(0))+1), ShAttrib1f(0.0), ShAttrib1f(0.0));
    bumpnorm = cond( min(verticalLimits, abs(tc(1))>mortarsize(1)), bumpnorm + normDeformation, bumpnorm);
    normDeformation = ShVector3f(ShAttrib1f(0.0),tc(1)/abs(tc(1)) * 0.5 * (tanh(10*abs(tc(1)-mortarsize(1)))+1), ShAttrib1f(0.0));
    bumpnorm = cond( min(horizontalLimits, abs(tc(0))<0.5-mortarsize(0)), bumpnorm + normDeformation, bumpnorm);
   
    bumpnorm = normalize(bumpnorm); // normalize the new normals
    
  } SH_END;
  
  /* Change the color of a brick
   * a noise function is used to add variations to the initial color
   */
  ShProgram brickModifier = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputAttrib1f changeColor;
    ShOutputColor3f brickVariations;  
    for(int i=0 ; i<3 ; i++) {
      brickVariations[i] = colorVariations(i) * cellnoise<1>(changeColor, false, false);
    }
    brickVariations += brick;
    
  } SH_END;
  
  /* Select the color to render in function of the id
   * id = 1.0 is a brick
   * id = 0.0 is the mortar
   */
  ShProgram select = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputAttrib1f id;
    ShInputColor3f brickVariations;
    ShInputVector3f half;
    ShInputVector3f light;
 
    ShOutputColor3f result;
  
    result= cond(id<0.5, mortar, brickVariations);

    // add Phong lighting
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);
    ShAttrib1f irrad = pos(normal | light);
    result = result * irrad + result * pow(pos(normal | half), ShAttrib1f(50.0)) / (normal | light);
         
  } SH_END;
 
  
  fsh = select <<
	  ((((keep<ShNormal3f>() & keep<ShAttrib1f>() & brickModifier) <<
	  (bumpmap & keep<ShAttrib1f>() & keep<ShAttrib1f>()) <<
	  brickID)) & keep<ShVector3f>() & keep<ShVector3f>());
  
  return true;
}

BrickID BrickID::instance = BrickID();
