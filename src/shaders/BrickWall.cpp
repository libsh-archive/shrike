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

/* hyperbolic tangent function
 * created because tanh in not yet implemented in Sh
 */
ShAttrib1f tanh( ShAttrib1f x) {
	return (pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1);
}
	


class BrickWall : public Shader {
public:
  BrickWall();
  ~BrickWall();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static BrickWall instance;
};

BrickWall::BrickWall()
  : Shader("Brick Wall: Brick Wall")
{
}

BrickWall::~BrickWall()
{
}

bool BrickWall::init()
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

	ShAttrib1f SH_DECL(noiseScale) = ShConstAttrib1f(0.1f);
	noiseScale.range(0.0f, 1.0f);

	ShAttrib1f SH_DECL(noiseFreq) = ShConstAttrib1f(25.0f);
	noiseFreq.range(0.0f, 100.0f);

	ShAttrib3f SH_DECL(noiseAmps) = ShConstAttrib3f(1.0f, 0.5f, 0.25f);	
	noiseAmps.range(0.0f, 1.0f);
 
  /* Create the bricks and the mortar between them
   */
  ShProgram brickID = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f pos;
    ShInOutTexCoord2f tc;
    ShInOutNormal3f norm;
    ShOutputAttrib1f id; // define if the current point belongs to a brick or to the mortar

    tc *= scale;
    tc(0) -= floor(tc(1))*offset; // change the horizontal position of a line
    tc(1) -= floor(tc(1)+0.5);
    tc(0) -= floor(tc(0)+0.5);

    id = min(abs(tc(0)) < 0.5-mortarsize(0), abs(tc(1)) > mortarsize(1)); // limits of a brick

  } SH_END;

  
  /* Add bump-mapping to the wall
   * edges are bumpmapped with a tanh function
   * bricks and mortar are bumpmapped with some noise
   */
  ShProgram bumpmap = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutTexCoord2f tc;
    ShInOutNormal3f norm;
 
		ShVector3f TilePerturb = noiseScale * sperlin<3>(tc * noiseFreq, noiseAmps, true);
		norm += lerp(ShAttrib1f(0.65), ShVector3f(0.0,0.0,0.0), TilePerturb);

    // set the limits of edges
    ShAttrib1f verticalLimits = min(abs(tc(0)) < 0.5-mortarsize(0)+0.01, abs(tc(0)) > 0.5-mortarsize(0)-0.01);
    ShAttrib1f horizontalLimits = min(abs(tc(1)) > mortarsize(1)-0.01, abs(tc(1)) < mortarsize(1)+0.01);
    // change the normals with a tanh function
    ShVector3f normDeformation = ShVector3f(tc(0)/abs(tc(0)) * 0.5 * (tanh(10*abs(tc(0))-0.5-mortarsize(0))+1), ShAttrib1f(0.0), ShAttrib1f(0.0));
    norm = cond( min(verticalLimits, abs(tc(1))>mortarsize(1)), norm + normDeformation, norm);
    normDeformation = ShVector3f(ShAttrib1f(0.0),tc(1)/abs(tc(1)) * 0.5 * (tanh(10*abs(tc(1)-mortarsize(1)))+1), ShAttrib1f(0.0));
    norm = cond( min(horizontalLimits, abs(tc(0))<0.5-mortarsize(0)), norm + normDeformation, norm);
       
    norm = normalize(norm); // normalize the new normal
    
  } SH_END;
  
  /* Change the color of a brick
   * a noise function is used to add variations to the initial color
   */
  ShProgram brickModifier = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc;
    ShOutputColor3f brickVariations;  
		brickVariations = noiseScale * sperlin<3>(tc * noiseFreq, noiseAmps, true);
		brickVariations = brick + lerp(ShAttrib1f(0.65), ShVector3f(0.0,0.0,0.0), brickVariations);
    
  } SH_END;
  
  /* Select the color to render in function of the id
   * id = 1.0 is a brick
   * id = 0.0 is the mortar
   */
  ShProgram select = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputColor3f brickVariations;
    ShInputNormal3f normal;
    ShInputAttrib1f id;
    ShInputVector3f half;
    ShInputVector3f light;
 
    ShOutputColor3f result;
  
    result= cond(id<0.5, mortar, brickVariations);

    // add Phong lighting
    normal = normalize(normal);
    half = normalize(half);
    light = normalize(light);
    result = result * pos(normal | light);
         
  } SH_END;
 
 fsh = select << (((brickModifier & keep<ShNormal3f>() & keep<ShAttrib1f>()) << (bumpmap & keep<ShAttrib1f>()) << brickID) & keep<ShVector3f>() & keep<ShVector3f>());
  
  return true;
}

BrickWall BrickWall::instance = BrickWall();
