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
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;


class HorizonMapping : public Shader {
public:
  HorizonMapping();
  ~HorizonMapping();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static HorizonMapping instance;
};

HorizonMapping::HorizonMapping()
  : Shader("Horizon Mapping: Horizon Mapping")
{
}

HorizonMapping::~HorizonMapping()
{
}

bool HorizonMapping::init()
{
	// load the image and put them in different textures
  ShImage image, horizmap1, horizmap2, dirmap1, dirmap2;
  image.loadPng(SHMEDIA_DIR "/horizonmaps/cross.png");
  horizmap1.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon1.png");
  horizmap2.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon2.png");
  
	ShTexture2D<ShVector3f> bump(image.width(),image.height());
  bump.memory(image.memory());
	bump.name("surface");
	ShTexture2D<ShColor4f> horizon1(horizmap1.width(), horizmap1.height());
  horizon1.memory(horizmap1.memory());
	horizon1.name("horizon maps");
  ShTexture2D<ShColor4f> horizon2(horizmap2.width(), horizmap2.height());
  horizon2.memory(horizmap2.memory());
	horizon2.name("horizon maps");

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
    onorm = Globals::mv | inorm; // Compute view-space normal
    otan = Globals::mv | itan;
		osurf = cross(onorm, otan);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

  } SH_END;

	ShAttrib1f SH_DECL(softness) = ShAttrib1f(10.0);
	softness.name("shadow softness");
	softness.range(1.0,30.0);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f normal;
    ShInputVector3f tangent;
		ShInputVector3f surface;
    ShInputVector3f light;
     
    ShOutputColor3f result;

    normal = normalize(normal);
    tangent = normalize(tangent);
    light = normalize(light);
    surface = normalize(surface);

		// compute the light vector in tangent coordinates
    ShAttrib1f lt = tangent | light;
    ShAttrib1f ls = surface | light;
    ShAttrib1f ln = normal | light;

    ShColor3f shadow = ShColor3f(0.5,0.5,0.5); // will substracted to the color to make shadows
    
		result = bump(u)/2 + shadow; // draw the elevation map
		
		ShAttrib1f normS = lt*lt+ls*ls; // for norm of light vector on the suface plane
    ShAttrib1f cosAngle = sqrt(normS) / sqrt(normS+ln*ln); // compute the light angle
		
		// get normalized values
    ShVector2f lightS = ShVector2f(lt,ls);
		lightS = normalize(lightS);
    lt = lightS(0); // cos(phi)
		ls = lightS(1); // sin(phi)

		ShAttrib1f b = 2*lt*lt-1; // 2*cos(phi)*cos(phi)-1
    ShAttrib1f bb = b*b;
		ShAttrib1f bb2 = 1.0-bb;
		ShAttrib1f null = 0.0;
		
		// define the basis functions
		ShAttrib1f b1 = cond( min(-lt, b), bb, null);
		ShAttrib1f b2 = cond( min(ls, -b), bb, null);
		ShAttrib1f b3 = cond( min(lt, b), bb, null);
		ShAttrib1f b4 = cond( min(-ls, -b), bb, null);
		ShAttrib1f b5 = cond( min(-lt, ls), bb2, null);
		ShAttrib1f b6 = cond( min(lt, ls), bb2, null);
		ShAttrib1f b7 = cond( min(lt, -ls), bb2, null);
		ShAttrib1f b8 = cond( min(-lt, -ls), bb2, null);

		// the interpolated horizon value
		ShAttrib1f cosHorizon = b1*horizon1(u)(0) + b2*horizon1(u)(1) + b3*horizon1(u)(2) + b4*horizon1(u)(3) +
														b5*horizon2(u)(0) + b6*horizon2(u)(1) +	b7*horizon2(u)(2) + b8*horizon2(u)(3);

		ShAttrib1f x = softness*abs(cosAngle-cosHorizon);
		shadow = ((pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1))*shadow; // use homemade tanh to create soft shadows
		
		result = cond( cosAngle>cosHorizon, result-shadow, result); // draw shadows in function of the angle
  
	} SH_END;
  return true;
}

HorizonMapping HorizonMapping::instance = HorizonMapping();


/* Used to draw the horizon maps calculated
 * A parameter can be changed to draw a specific direction
 */
class ViewHorizonMaps : public Shader {
public:
  ViewHorizonMaps();
  ~ViewHorizonMaps();

  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static ViewHorizonMaps instance;
};

ViewHorizonMaps::ViewHorizonMaps()
  : Shader("Horizon Mapping: Horizon Maps")
{
}

ViewHorizonMaps::~ViewHorizonMaps()
{
}

bool ViewHorizonMaps::init()
{
  ShImage horizmap1, horizmap2;
  horizmap1.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon1.png");
  horizmap2.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon2.png");
  
	ShTexture2D<ShColor4f> horizon1(horizmap1.width(), horizmap1.height());
  horizon1.memory(horizmap1.memory());
  ShTexture2D<ShColor4f> horizon2(horizmap2.width(), horizmap2.height());
  horizon2.memory(horizmap2.memory());
  
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;

	ShColor3f SH_DECL(shadowcolor) = ShColor3f(1.0,1.0,0.0);
  ShAttrib1f SH_DECL(direction) = ShAttrib1f(1.0);
  direction.range(1.0,9.0);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
     
    ShOutputColor3f result;

		// draw a direction in function of the parameter "direction"
		result = horizon1(u)(0)*shadowcolor;
		result = cond(direction < 1.5, result, horizon2(u)(0)*shadowcolor);
		result = cond(direction < 2.5, result, horizon1(u)(1)*shadowcolor);
		result = cond(direction < 3.5, result, horizon2(u)(1)*shadowcolor);
		result = cond(direction < 4.5, result, horizon1(u)(2)*shadowcolor);
		result = cond(direction < 5.5, result, horizon2(u)(2)*shadowcolor);
		result = cond(direction < 6.5, result, horizon1(u)(3)*shadowcolor);
		result = cond(direction < 7.5, result, horizon2(u)(3)*shadowcolor);
		result = cond(direction < 8.5, result, (horizon1(u)(0)+horizon1(u)(1)+horizon1(u)(2)+horizon1(u)(3)+
																						horizon2(u)(0)+horizon2(u)(1)+horizon2(u)(2)+horizon2(u)(3))/8.0*shadowcolor);
		result = shadowcolor - result; // to get black where there is no horizon value

  } SH_END;
  return true;
}

ViewHorizonMaps ViewHorizonMaps::instance = ViewHorizonMaps();
