#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <math.h>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class BrickHorizon : public Shader {
public:
  BrickHorizon();
  ~BrickHorizon();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static BrickHorizon instance;
};

BrickHorizon::BrickHorizon()
  : Shader("Brick Wall: Horizon Maps")
{
}

BrickHorizon::~BrickHorizon()
{
}

bool BrickHorizon::init()
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

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = normalize(Globals::mv | inorm); // Compute view-space normal
		otan = normalize(Globals::mv | itan); // Compute the view-space tangent
		osurf = cross(onorm, otan);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

  } SH_END;

  ShColor3f brick = ShColor3f(0.7, 0.1, 0.1);
  ShColor3f mortar = ShColor3f(0.3, 0.3, 0.3);

  ShAttrib2f SH_DECL(scale) = ShAttrib2f(3.0, 5.0);
  scale.name("brick size");
  scale.range(1.0, 100.0);

  ShAttrib2f SH_DECL(mortarsize) = ShAttrib2f(0.03, 0.03);
  mortarsize.name("mortar size");
  mortarsize.range(0.0, 0.1);

  ShAttrib1f SH_DECL(offset) = ShAttrib1f(0.5);
 
	ShAttrib3f SH_DECL(colorVariations) = ShAttrib3f(0.2, 0.1, 0.1);
  colorVariations.range(0.0, 1.0);
	
	ShAttrib1f SH_DECL(brickHeight) = ShAttrib1f(0.1);
	brickHeight.name("brick height");
	brickHeight.range(0.01,0.1); 
	
	ShAttrib1f SH_DECL(intensity) = ShAttrib1f(1.0);
	intensity.name("shadow intensity");
	intensity.range(0.1,10.0);

  // Create the bricks and the mortar between them
  ShProgram brickID = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f pos;
		ShInOutTexCoord2f tc;
		ShOutputAttrib1f changeNorm;
		ShInOutNormal3f normal;
		ShOutputAttrib1f id; // define if the current point belongs to a brick or to the mortar
    ShOutputAttrib1f changeColor; // used to generate different colors

    tc *= scale;
		tc += 2*mortarsize;
    changeNorm = 0.1*pos(0)*pos(1)*tc(0)*tc(1);
    tc(0) -= floor(tc(1))*offset; // change the horizontal position of a line
    changeColor = abs(floor(tc(1))) * abs(floor(tc(0))); // change the color of each brick
    tc(1) -= floor(tc(1));
    tc(0) -= floor(tc(0));
    id = min(abs(tc(0)) > 2*mortarsize(0), abs(tc(1)) > 2*mortarsize(1)); // limits of a brick
  } SH_END;

	/* Add bump-mapping to the wall
   * bricks and mortar are bumpmapped with some noise
   */
  ShProgram bumpmap = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInputAttrib1f changeNorm;
    ShInOutNormal3f normal;
    
    for(int i=0 ; i<2 ; i++) {
      normal[i] += 0.05*cellnoise<1>(changeNorm, false, false); // add noise to make a rough surface
    }
	  //normal = normalize(normal); // normalize the new normal
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
		ShInOutTexCoord2f tc;
    ShInOutNormal3f normal;
    ShInputAttrib1f id;
    ShInputColor3f brickVariations;
		ShInOutVector3f tangent;
		ShInOutVector3f surface;
    ShInOutVector3f light;
 
    ShOutputColor3f result;
  
    result= cond(id, brickVariations, mortar);
    normal = normalize(normal);
    light = normalize(light);
    result = result * pos(normal | light);
         
  } SH_END;

	
	/* Add horizon mapping to the bricks
	 */
  ShProgram horizonmapping = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInputTexCoord2f tc;
    ShInputNormal3f normal;
		ShInputVector3f tangent;
		ShInputVector3f surface;
		ShInputVector3f light;
		ShInOutColor3f result;
		
		ShColor3f shadow = ShColor3f(0.5,0.5,0.5);
		
		ShAttrib1f lt = light | tangent;
		ShAttrib1f ls = light | surface;
		ShAttrib1f ln = light | normal;

		ShAttrib1f cosAngle = sqrt(lt*lt+ls*ls) / sqrt(lt*lt+ls*ls+ln*ln);
		
		ShVector2f lightS = ShVector2f(lt,ls);
		lightS = normalize(lightS);
		lt = lightS(0);
		ls = lightS(1);
		
		ShAttrib1f b = 2.0*lt*lt-1;
		ShAttrib1f bb = b*b;
		
		ShAttrib2f b1,b2,b3,b4,b5,b6,b7,b8; // the basis functions used to blend
		b1 = cond( min( min(lt<0.0, b>0.0), ls>0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b2 = cond( min( min(lt<0.0, b<0.0), ls>0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b3 = cond( min( min(lt>0.0, b<0.0), ls>0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b4 = cond( min( min(lt>0.0, b>0.0), ls>0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b5 = cond( min( min(lt>0.0, b>0.0), ls<0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b6 = cond( min( min(lt>0.0, b<0.0), ls<0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b7 = cond( min( min(lt<0.0, b<0.0), ls<0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));
		b8 = cond( min( min(lt<0.0, b>0.0), ls<0.0), ShAttrib2f(bb,1.0-bb), ShAttrib2f(0.0,0.0));


		ShAttrib1f brickHeight2 = brickHeight*brickHeight;
		ShAttrib1f horizonLoc; // position of the horizon
	
		// create a horizon value for a brick, to make some "holes" in the surface
		ShAttrib1f hole = 1.0-0.4*cellnoise<1>(10000*tc(0)*tc(1), false, false); 
		
		ShAttrib1f horizon1 = hole, horizon2 = hole, horizon3 = hole, horizon4 = hole,
							 horizon5 = hole, horizon6 = hole, horizon7 = hole, horizon8 = hole;
		
//		ShAttrib1f horizon1 = 1.0, horizon2 = 1.0, horizon3 = 1.0, horizon4 = 1.0,
//							 horizon5 = 1.0, horizon6 = 1.0, horizon7 = 1.0, horizon8 = 1.0;
		
		horizonLoc = min(tc(0)<2*mortarsize(0),tc(1)>2*mortarsize(1));
		horizon1 = cond(horizonLoc, tc(0)/sqrt(tc(0)*tc(0)+brickHeight*brickHeight), horizon1);
		
		ShAttrib2f tc2 = ShAttrib2f(2*mortarsize(0)-tc(0),1.0+2*mortarsize(1)-tc(1));
		horizon3 = cond(horizonLoc, tc2(0)/sqrt(tc2(0)*tc2(0)+brickHeight2), horizon3);

		horizonLoc = max(tc(0)<2*mortarsize(0), min(tc(0)>2*mortarsize(0), tc(1)<2*mortarsize(1)));
		horizon2 = cond(horizonLoc, tc(1)/sqrt(tc(1)*tc(1)+brickHeight2), horizon2);
		horizonLoc = min(tc(1)<2*mortarsize(1), min(tc(0)>offset, tc(0)<offset+2*mortarsize(0)));
		horizon2 = cond(horizonLoc, tc2(1)/sqrt(tc2(1)*tc2(1)+brickHeight2), horizon2);
		
		horizon4 = cond(tc(0)<2*mortarsize(0), tc2(1)/sqrt(tc2(1)*tc2(1)+brickHeight2), horizon4);
		tc2[1] = 2*mortarsize(1)-tc(1);
		horizonLoc = min(tc(1)<2*mortarsize(1),tc(0)>2*mortarsize(0));
		horizon4 = cond(horizonLoc, tc2(1)/sqrt(tc2(1)*tc2(1)+brickHeight2), horizon4);
	
		
		// the diagonals
		// part5
		ShAttrib1f diag;
		diag = 2*tc(0)*tc(0);
		horizonLoc = min(tc(0)<2*mortarsize(0), tc(1)>2*mortarsize(1));
		horizon5 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon5);
		horizon8 = horizon5; // same orientation for the 8th part
		diag = 2*tc(1)*tc(1);
		horizonLoc = tc(1)<2*mortarsize(1);
		horizon5 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon5);
		horizon6 = horizon5; // same orientation for the 6th part
		horizonLoc = min(tc(0)<2*mortarsize(0), min(tc(1)>=2*mortarsize(1), tc(1)<=tc(0)+2*mortarsize(1)));
		horizon5 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon5);
		diag = tc(0)-1.0+offset;	
		diag *= 2*diag;
		horizonLoc = min(tc(1)<2*mortarsize(1), min(tc(0)>=1.0-offset+tc(1), tc(0)<=1.0-offset+tc(1)+2*mortarsize(0)));
		horizon5 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon5);

		// part 6
		diag = 2*mortarsize(0)-tc(0);
		diag *= 2*diag;
		horizonLoc = min(tc(0)<2*mortarsize(0), tc(1)>2*mortarsize(1));
		horizon6 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon6);
		horizon7 = horizon6; // same orientation for the 7th part
		diag = 2*tc(1)*tc(1);
		horizonLoc = min(tc(0)<2*mortarsize(0), min(tc(1)>=2*mortarsize(1), tc(1)<=-tc(0)+2*mortarsize(0)+2*mortarsize(1)));
		horizon6 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon6);
		diag = 1.0+2*mortarsize(0)-tc(0)-offset;
		diag *= 2*diag;
		horizonLoc = min(tc(1)<2*mortarsize(1), min(tc(0)>=1.0-offset-tc(1), tc(0)<=1.0-offset-tc(1)+2*mortarsize(0)));
		horizon6 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon6);
		
		// part 7
		diag = 2*mortarsize(1)-tc(1);
		diag *= 2*diag;
		horizonLoc = tc(1)<2*mortarsize(1);
		horizon7 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon7);
		diag = 1.0+2*mortarsize(0)-tc(0);
		diag *= 2*diag;
		horizonLoc = min(tc(1)<2*mortarsize(1), min(tc(0)>1.0-2*mortarsize(1)+tc(1),tc(0)<1.0+2*mortarsize(0)-2*mortarsize(1)+tc(1)));
		horizon7 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon7);
		diag = 2*mortarsize(0)-tc(0);
		diag *= 2*diag;
		horizonLoc = min(tc(1)<2*mortarsize(1), tc(0)<tc(1)+2*mortarsize(0)-2*mortarsize(1));
		horizon7 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon7);
		diag = 1.0-tc(1)+2*mortarsize(1);
		diag *= 2*diag;
		horizonLoc = min(tc(0)<2*mortarsize(0), tc(1)>=1.0+tc(0)-2*mortarsize(0));
		horizon7 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon7);
		
		// part 8
		diag = 2*mortarsize(1)-tc(1);
		diag *= 2*diag;
		horizonLoc = tc(1)<2*mortarsize(1);
		horizon8 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon8);
		diag = 2*tc(0)*tc(0);
		horizonLoc = min(tc(1)<2*mortarsize(1), min(tc(0)>=2*mortarsize(1)-tc(1), tc(0)<=2*mortarsize(0)+2*mortarsize(1)-tc(1)));
		horizon8 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon8);
		diag = 1.0-tc(1)+2*mortarsize(1);
		diag *= 2*diag;
		horizonLoc = min(tc(0)<2*mortarsize(0), tc(1)>=1.0-tc(0));
		horizon8 = cond(horizonLoc, sqrt(diag)/sqrt(diag+brickHeight2), horizon8);
		
		// interpolate the horizon with the 8 values found
		ShAttrib1f cosHorizon = b1(0)*horizon1 + b1(1)*horizon5 +
														b2(0)*horizon2 + b2(1)*horizon5 +
														b3(0)*horizon2 + b3(1)*horizon6 +
														b4(0)*horizon3 + b4(1)*horizon6 +
														b5(0)*horizon3 + b5(1)*horizon7 +
														b6(0)*horizon4 + b6(1)*horizon7 +
														b7(0)*horizon4 + b7(1)*horizon8 +
														b8(0)*horizon1 + b8(1)*horizon8;
		
		ShAttrib1f x = abs(cosAngle-cosHorizon)*intensity;
		shadow = ((pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1))*shadow; // use tanh to create soft shadows
		result = cond( cosAngle>cosHorizon, result-shadow, result); // draw shadows in function of the angle

  } SH_END;
 
 fsh = horizonmapping << (select << (((keep<ShTexCoord2f>() & keep<ShNormal3f>() & keep<ShAttrib1f>() & brickModifier) << (keep<ShTexCoord2f>() & bumpmap & keep<ShAttrib1f>() & keep<ShAttrib1f>()) << brickID) & keep<ShVector3f>() & keep<ShVector3f>() & keep<ShVector3f>()));
  return true;
}

BrickHorizon BrickHorizon::instance = BrickHorizon();
