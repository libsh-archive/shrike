// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////
#include <sh/sh.hpp>
#include <shutil/shutil.hpp>
#include <iostream>
#include <math.h>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

/* hyperbolic tangent function
 * created because tanh in not yet implemented in Sh
 */
ShAttrib1f tanh( ShAttrib1f x) 
{
  return (pow(M_E, 2*x) - 1) / (pow(M_E, 2*x) + 1);
}


class BrickWall : public Shader {
public:
  BrickWall(const Globals&);
  ~BrickWall();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

BrickWall::BrickWall(const Globals& globals)
  : Shader("Brick Wall: Brick Wall", globals)
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

    opos = m_globals.mvp | ipos; // Compute NDC position
    onorm = m_globals.mv | inorm; // Compute view-space normal

    ShPoint3f posv = (m_globals.mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(m_globals.lightPos - posv); // Compute light direction

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
    ShOutputAttrib1f changeColor; // change the color between 2 consecutive bricks
    ShInOutNormal3f norm;
    ShOutputAttrib1f id; // define if the current point belongs to a brick or to the mortar
    tc *= scale;
    tc(0) -= floor(tc(1))*offset; // change the horizontal position of a line
    changeColor = abs(floor(tc(1))) * abs(floor(tc(0)+0.5));
    tc(1) -= floor(tc(1)+0.5);
    tc(0) -= floor(tc(0)+0.5);

    id = SH::min(abs(tc(0)) < 0.5-mortarsize(0), abs(tc(1)) > mortarsize(1)); // limits of a brick

  } SH_END;

  
  /* Add bump-mapping to the wall
   * edges are bumpmapped with a tanh function
   * bricks and mortar are bumpmapped with some noise
   */
  ShProgram bumpmap = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInOutTexCoord2f tc;
    ShInOutAttrib1f changeColor;
    ShInOutNormal3f norm;
 
    ShVector3f TilePerturb = noiseScale * sperlin<3>(tc * noiseFreq, noiseAmps, true);
    norm = mad(0.35, TilePerturb, norm);

    // set the limits of edges
    ShAttrib1f verticalLimits = SH::min(abs(tc(0)) < 0.5-mortarsize(0)+0.01, abs(tc(0)) > 0.5-mortarsize(0)-0.01);
    ShAttrib1f horizontalLimits = SH::min(abs(tc(1)) > mortarsize(1)-0.01, abs(tc(1)) < mortarsize(1)+0.01);
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

void create(ShaderList &list, const Globals &globals) {
  list.push_back(new BrickWall(globals));
}

#ifdef SHRIKE_LIBRARY_SHADER
extern "C" {
  SHRIKE_DLLEXPORT
  ShaderList shrike_library_create(const Globals &globals) {
    ShaderList list;
    create(list, globals);
    //list.push_back(new BrickWall(globals));
    return list;
  }
}
#else
static StaticLinkedShader<BrickWall> instance = 
       StaticLinkedShader<BrickWall>();
#endif
