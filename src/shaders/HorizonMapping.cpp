#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#define SIZE_BUMP 10

class HorizonMaps : public Shader {
public:
  HorizonMaps();
  ~HorizonMaps();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static HorizonMaps instance;
};

HorizonMaps::HorizonMaps()
  : Shader("Horizon Mapping: Horizon Mapping")
{
}

HorizonMaps::~HorizonMaps()
{
}

void getHorizon(
  const ShImage& image, // input
  ShImage& horizonmap1,
  ShImage& horizonmap2
  ) {
  for(int i=0 ; i<image.width() ; i++) {
    for(int j=0; j<image.height() ; j++) {
      float angle;
      int u,v;
      horizonmap1(i, j, 0) = 0;
      horizonmap1(i, j, 1) = 0;
      horizonmap1(i, j, 2) = 0;
      horizonmap1(i, j, 3) = 0;
      horizonmap2(i, j, 0) = 0;
      horizonmap2(i, j, 1) = 0;
      horizonmap2(i, j, 2) = 0;
      horizonmap2(i, j, 3) = 0;
      angle = 0.0;
      u=i;
      while(u>0) {
	u--;
	angle = atan(SIZE_BUMP*(image(u, j, 0)-image(i, j, 0)) / (i-u));
	if(angle > horizonmap1(i, j, 0)) {
	  horizonmap1(i, j, 0) = angle;
	}
      }
      angle = 0.0;
      v = j;
      while(v>0) {
	v--;
	angle = atan(SIZE_BUMP*(image(i, v, 0)-image(i, j, 0)) / (j-v));
	if(angle > horizonmap1(i, j, 1)) {
	  horizonmap1(i, j, 1) = angle;
	}
      }
      angle = 0.0;
      u = i;
      while(u<image.width()) {
	u++;
	angle = atan(SIZE_BUMP*(image(u, j, 0)-image(i, j, 0)) / (u-i));
	if(angle > horizonmap1(i, j, 2)) {
	  horizonmap1(i, j, 2) = angle;
	}
      }
      angle = 0.0;
      v = j;
      while(v<image.height()) {
	v++;
	angle = atan(SIZE_BUMP*(image(i, v, 0)-image(i, j, 0)) / (v-j));
	if(angle > horizonmap1(i, j, 3)) {
	  horizonmap1(i, j, 3) = angle;
	}
      }
      angle = 0.0;
      u = i;v = j;
      while(u>0 && v>0) {
	u--;
	v--;
        angle = atan(SIZE_BUMP*(image(u, v, 0)-image(i, j, 0)) / sqrt((i-u)*(i-u)+(j-v)*(j-v)));
	if(angle > horizonmap2(i, j, 0)) {
	  horizonmap2(i, j, 0) = angle;
	}
      }
      u = i;v = j;
      while(u<image.width() && v>0) {
        u++;
	v--;
        angle = atan(SIZE_BUMP*(image(u, v, 0)-image(i, j, 0)) / sqrt((u-i)*(u-i)+(j-v)*(j-v)));
	if(angle > horizonmap2(i, j, 1)) {
	  horizonmap2(i, j, 1) = angle;
	}
      }
      u = i;v = j;
      while(u>0 && v<image.height()) {
        u--;
	v++;
        angle = atan(SIZE_BUMP*(image(u, v, 0)-image(i, j, 0)) / sqrt((u-i)*(u-i)+(j-v)*(j-v)));
	if(angle > horizonmap2(i, j, 2)) {
	  horizonmap2(i, j, 2) = angle;
	}
      }
      u = i;v = j;
      while(u<image.width() && v<image.height()) {
        u++;
	v++;
        angle = atan(SIZE_BUMP*(image(u, v, 0)-image(i, j, 0)) / sqrt((u-i)*(u-i)+(j-v)*(j-v)));
	if(angle > horizonmap2(i, j, 3)) {
	  horizonmap2(i, j, 3) = angle;
	}
      }
    }
  }
}

bool HorizonMaps::init()
{
  ShImage image;
  //image.loadPng(SHMEDIA_DIR "/testhorizon.png");
  image.loadPng(SHMEDIA_DIR "/bumpmaps/bumps.png");
  ShTexture2D<ShVector3f> bump(image.width(),image.height());
  bump.memory(image.memory());
  ShTexture2D<ShColor4f> horizon1(image.width(), image.height());
  ShTexture2D<ShColor4f> horizon2(image.width(), image.height());
  ShImage horizmap1(image.width(), image.height(), 4); // N S E W
  ShImage horizmap2(image.width(), image.height(), 4);// NE SE SW NW
  getHorizon(image, horizmap1, horizmap2);
  horizon1.memory(horizmap1.memory());
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

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f normal;
    ShInputVector3f tangent;
    ShInputVector3f light;
     
    ShOutputColor3f result;

    normal = normalize(normal);
    tangent = normalize(tangent);
    ShVector3f surface = cross(normal, tangent);
    surface = normalize(surface);
    light = normalize(light);

    
     
    ShVector3f bu = bump(u) - ShAttrib3f(0.5,0.5,0.0);
    bu *= scale;
    ShVector3f bn = tangent * bu(0) + surface * bu(1) + normal * bu(2);
    
    
    ShAttrib1f lt = tangent | light;
    ShAttrib1f ls = surface | light;
    ShAttrib1f ln = normal | light;
    ShVector2f lightS = ShVector2f(lt,ls);
    
    //result = ShColor3f(bump(u)(0)/2+0.5, bump(u)(1)/2+0.5, bump(u)(2)/2+0.5); // draw the bumps
    result = (bn | light) * diffuse;

    ShColor3f Shadow = ShColor3f(0.5,0.5,0.5);

    ShAttrib1f b = (2*lightS(0)*lightS(0)-1);
    ShAttrib1f b2 = b*b;
    ShAttrib1f angle = acos(-lt / sqrt(lt*lt+ln*ln));
    ShAttrib1f shadowed = cond( min(angle<horizon1(u)(0), min(b>ShAttrib1f(0.0), -lt>ShAttrib1f(0.5))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    angle = acos(ls / sqrt(ls*ls+ln*ln));
    shadowed = cond( min(angle<horizon1(u)(1), min(b<ShAttrib1f(-0.5), -ls<ShAttrib1f(0.0))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    angle = acos(lt / sqrt(lt*lt+ln*ln));
    shadowed = cond( min(angle<horizon1(u)(2), min(b>ShAttrib1f(0.0), lt>ShAttrib1f(0.5))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    angle = acos(-ls / sqrt(ls*ls+ln*ln));
    shadowed = cond( min(angle<horizon1(u)(3), min(b<ShAttrib1f(-0.5), ls<ShAttrib1f(0.0))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    angle = acos(sqrt(lt*lt+ls*ls) / sqrt(lt*lt+ls*ls+ln*ln));
    b2 = 1-b2;
    shadowed = cond( min(angle<horizon2(u)(0), min(lt<ShAttrib1f(0.0), min(abs(b)<ShAttrib1f(0.5), ls>ShAttrib1f(0.0)))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);

    shadowed = cond( min(angle<horizon2(u)(1), min(lt>ShAttrib1f(0.0), min(abs(b)<ShAttrib1f(0.5), ls>ShAttrib1f(0.0)))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    shadowed = cond( min(angle<horizon2(u)(2), min(lt<ShAttrib1f(0.0), min(abs(b)<ShAttrib1f(0.5), ls<ShAttrib1f(0.0)))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);
    
    shadowed = cond( min(angle<horizon2(u)(3), min(lt>ShAttrib1f(0.0), min(abs(b)<ShAttrib1f(0.5), ls<ShAttrib1f(0.0)))), ShAttrib1f(1.0), ShAttrib1f(0.0));
    result = cond( shadowed, result-b2*Shadow, result);

    
  } SH_END;
  return true;
}

HorizonMaps HorizonMaps::instance = HorizonMaps();



