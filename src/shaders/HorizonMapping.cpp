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

  void createMaps(const ShImage&, ShImage&, ShImage&);
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
  ShImage image, horizmap1, horizmap2, dirmap1, dirmap2;
  image.loadPng(SHMEDIA_DIR "/horizonmaps/cross.png");
  horizmap1.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon1.png");
  horizmap2.loadPng(SHMEDIA_DIR "/horizonmaps/cross_horizon2.png");
  
  ShTexture2D<ShVector3f> bump(image.width(),image.height());
  bump.memory(image.memory());
	ShTexture2D<ShColor4f> horizon1(image.width(), image.height());
  horizon1.memory(horizmap1.memory());
  ShTexture2D<ShColor4f> horizon2(image.width(), image.height());
  horizon2.memory(horizmap2.memory());

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    ShOutputVector3f otan;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    otan = Globals::mv | itan;
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

  } SH_END;

	ShColor3f SH_DECL(diffuse) = ShColor3f(1.0,0.0,0.0);
  ShAttrib3f SH_DECL(scale) = ShAttrib3f(2.0,2.0,1.0);
  scale.range(0.0f,20.0f);
	
	ShAttrib1f SH_DECL(intensity) = ShAttrib1f(10.0);
	intensity.name("shadow intensity");
	intensity.range(1.0,30.0);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f normal;
    ShInputVector3f tangent;
    ShInputVector3f light;
     
    ShOutputColor3f result;

    normal = normalize(normal);
    tangent = normalize(tangent);
    light = normalize(light);
    
		ShVector3f surface = cross(normal, tangent); // compute the surface vector
    surface = normalize(surface);

    // create the bumps by changing the normal 
    ShVector3f bu = bump(u) - ShAttrib3f(0.5,0.5,0.0);
    bu *= scale;
    ShVector3f bn = tangent * bu(0) + surface * bu(1) + normal * bu(2);
    
		// compute the light vector in tangent coordinates
    ShAttrib1f lt = tangent | light;
    ShAttrib1f ls = surface | light;
    ShAttrib1f ln = normal | light;

    result = ShColor3f(bump(u)(0)/2+0.5, bump(u)(1)/2+0.5, bump(u)(2)/2+0.5); // draw the bumps
    //result = (bn | light) * diffuse; // draw the bumps

    ShColor3f shadow = ShColor3f(0.5,0.5,0.5); // substracted to the color to make shadows

    ShAttrib1f cosAngle = sqrt(lt*lt+ls*ls) / sqrt(lt*lt+ls*ls+ln*ln); // compute the light angle
		
		// get the normalized values
    ShVector2f lightS = ShVector2f(lt,ls);
		lightS = normalize(lightS);
    lt = lightS(0); // cos(phi)
		ls = lightS(1); // sin(phi)

		ShAttrib1f b = 2*lt*lt-1; // 2*cos(phi)*cos(phi)-1
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

		// the interpolated horizon value
		ShAttrib1f cosHorizon = b1(0)*horizon1(u)(0) + b1(1)*horizon2(u)(0) +
														b2(0)*horizon1(u)(1) + b2(1)*horizon2(u)(0) +
														b3(0)*horizon1(u)(1) + b3(1)*horizon2(u)(1) +
														b4(0)*horizon1(u)(2) + b4(1)*horizon2(u)(1) +
														b5(0)*horizon1(u)(2) + b5(1)*horizon2(u)(2) +
														b6(0)*horizon1(u)(3) + b6(1)*horizon2(u)(2) +
														b7(0)*horizon1(u)(3) + b7(1)*horizon2(u)(3) +
														b8(0)*horizon1(u)(0) + b8(1)*horizon2(u)(3);

	
		ShAttrib1f x = abs(cosAngle-cosHorizon)*intensity;
		shadow = ((pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1))*shadow; // use tanh to create soft shadows
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
  horizmap1.loadPng(SHMEDIA_DIR "/horizonmaps/bricks_horizon1.png");
  horizmap2.loadPng(SHMEDIA_DIR "/horizonmaps/bricks_horizon2.png");
  
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
  direction.range(1.0,8.0);

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
		result = shadowcolor - result; // to get black where there is no horizon value

  } SH_END;
  return true;
}

ViewHorizonMaps ViewHorizonMaps::instance = ViewHorizonMaps();

