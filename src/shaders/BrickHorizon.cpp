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

	ShAttrib1f SH_DECL(noiseScale) = ShConstAttrib1f(0.15f);
	noiseScale.range(0.0f, 1.0f);

	ShAttrib1f SH_DECL(noiseFreq) = ShConstAttrib1f(20.0f);
	noiseFreq.range(0.0f, 100.0f);

	ShAttrib1f SH_DECL(noiseAmps) = ShConstAttrib1f(1.0f);	
	noiseAmps.range(0.0f, 1.0f);

  // Create the bricks and the mortar between them
  ShProgram brickID = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f pos;
		ShInOutTexCoord2f tc;
    ShOutputAttrib1f changeColor; // change the color between 2 consecutive bricks
		ShInOutNormal3f normal;
		ShOutputAttrib1f id; // define if the current point belongs to a brick or to the mortar

    tc *= scale;
		tc += 2*mortarsize;
    tc(0) -= floor(tc(1))*offset; // change the horizontal position of a line
    changeColor = abs(floor(tc(1))) * abs(floor(tc(0)));
    tc(1) -= floor(tc(1));
    tc(0) -= floor(tc(0));
		
    id = min(abs(tc(0)) > 2*mortarsize(0), abs(tc(1)) > 2*mortarsize(1)); // limits of a brick
		
  } SH_END;

	/* Add bump-mapping to the wall
   * bricks and mortar are bumpmapped with some noise
   */
  ShProgram bumpmap = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutTexCoord2f tc;
    ShInOutAttrib1f changeColor;
    ShInOutNormal3f normal;
    
    ShVector3f TilePerturb = noiseScale * sperlin<3>(tc * noiseFreq, noiseAmps, true);
    normal = mad(0.35, TilePerturb, normal);
    
    normal = normalize(normal);
		
  } SH_END;

	/* Change the color of a brick
   * a noise function is used to add variations to the initial color
   */
  ShProgram brickModifier = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutTexCoord2f tc;
    ShInputAttrib1f changeColor;
    ShOutputColor3f brickVariations; 
    brickVariations = noiseScale * perlin<3>(tc * noiseFreq, noiseAmps, true);
    brickVariations = mad(cellnoise<1>(changeColor, false), colorVariations, brickVariations);
    brickVariations = mad(0.35, brickVariations, brick);
  } SH_END;

  /* Select the color to render in function of the id
   * id = 1.0 is a brick
   * id = 0.0 is the mortar
   */
  ShProgram select = SH_BEGIN_PROGRAM("gpu:fragment") {
		ShInOutTexCoord2f tc;
    ShInputColor3f brickVariations;
    ShInOutNormal3f normal;
    ShInputAttrib1f id;
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

		ShAttrib1f normS = lt*lt+ls*ls;
		ShAttrib1f cosAngle = sqrt(normS) / sqrt(normS+ln*ln);
		
		ShVector2f lightS = ShVector2f(lt,ls);
		lightS = normalize(lightS);
		lt = lightS(0);
		ls = lightS(1);
		
		ShAttrib1f b = 2.0*lt*lt-1;
		ShAttrib1f bb = b*b;
		ShAttrib1f bb2 = 1.0-bb;
		
		ShAttrib1f brickHeight2 = brickHeight*brickHeight;
		ShAttrib1f horizonLoc; // position of the horizon
	
		// create a horizon value for a brick, to make some "holes" in the surface
		ShAttrib1f hole = 1.0 - noiseScale * sperlin<1>(tc *2.0 * noiseFreq, noiseAmps, true);
		
		ShAttrib1f horizon1 = hole, horizon2 = hole, horizon3 = hole, horizon4 = hole,
							 horizon5 = hole, horizon6 = hole, horizon7 = hole, horizon8 = hole;
		
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
		
		// define the basis functions
		ShAttrib1f null = 0.0;
		ShAttrib1f b1 = cond( min(-lt, b), bb, null);
		ShAttrib1f b2 = cond( min(ls, -b), bb, null);
		ShAttrib1f b3 = cond( min(lt, b), bb, null);
		ShAttrib1f b4 = cond( min(-ls, -b), bb, null);
		ShAttrib1f b5 = cond( min(-lt, ls), bb2, null);
		ShAttrib1f b6 = cond( min(lt, ls), bb2, null);
		ShAttrib1f b7 = cond( min(lt, -ls), bb2, null);
		ShAttrib1f b8 = cond( min(-lt, -ls), bb2, null);

		// the interpolated horizon value
		ShAttrib1f cosHorizon = b1*horizon1 + b2*horizon2 + b3*horizon3 + b4*horizon4 +
														b5*horizon5 + b6*horizon6 +	b7*horizon7 + b8*horizon8;
	
		ShAttrib1f x = abs(cosAngle-cosHorizon)*intensity;
		shadow = ((pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1))*shadow; // use tanh to create soft shadows
		result = cond( cosAngle>cosHorizon, result-shadow, result); // draw shadows in function of the angle

  } SH_END;
 
 fsh = horizonmapping << (select << (((brickModifier & keep<ShNormal3f>() & keep<ShAttrib1f>()) << (bumpmap & keep<ShAttrib1f>()) << brickID) & keep<ShVector3f>() & keep<ShVector3f>() & keep<ShVector3f>()));
  return true;
}

BrickHorizon BrickHorizon::instance = BrickHorizon();
