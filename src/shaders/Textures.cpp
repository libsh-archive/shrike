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
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace SH;
using namespace ShUtil;

class Textures : public Shader {
public:
  Textures();
  ~Textures();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static Textures instance;
};

Textures::Textures()
  : Shader("Textures: Simple")
{
}

Textures::~Textures()
{
}

bool Textures::init()
{
  // This vertex program is _very_ simple.
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos; // Position in WC
    
    ShOutputPosition4f opos; // Position in NDC

    // InOut types are very useful sometimes, if you want to somehow
    // modify or just pass through some attribute.
    // The following line of code is essentially equivalent to:
    //   ShInputTexCoord2f itc;
    //   ShOutputTexCoord2f tc = itc;
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;

  // Declare an image on the host. ShImage can load and save PNG
  // files, and manages our memory for us.
  ShImage image;

  // This will load the given PNG file and allocate a memory for the image.
  load_PNG(image, normalize_path(SHMEDIA_DIR "/textures/abcd.png"));

  // Here we could be using...
  // ShArray: nearest-neighbour lookup, no mipmapping
  // ShTable: linear interpolation, no mipmapping
  // ShTexture: linear interpolation and mipmapping
  ShTable2D<ShColor3fub> texture(image.width(), image.height());

  // You can also wrap the above type in ShWrapClamp<> or ShWrapRepeat<>
  // for different edge wrapping styles. ShWrapClamp is the default.
  // ShUnclamped<> can be used to indicate unclamped floating-point
  // textures, but this is very platform-dependant.
  // ShClamped<> is the default, texture values are clamped when
  // transferred to the GPU.

  // Here we are saying that we want the texture to use the same
  // "memory" as the image. If the image changes, so will the texture
  // (but only if we rebind our shaders).
  texture.memory(image.memory());

  // Now the fragment program. Not a very hard one either.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // Get the texture coordinate, passed
                          // through by the vertex program.
    
    ShOutputColor3f result; // This is what we'll show on the screen.

    // This is a texture lookup.
    // texture(tc) indicates that tc lies between (0,0) and (1,1).
    // texture[tc] indicates that tc lies between (0,0) and
    // (width,height), which is useful on some occasions and may be
    // more efficient for certain texture types (e.g. Rect textures).
    result = texture(tc);
  } SH_END;

  // Note that textures are internally reference counted. Even though
  // the above texture is declared in this scope, its real
  // representation won't go away when we return from this function,
  // because fsh uses it (and hence refers to it).
  // This takes a lot of effort away from the user.
  return true;
}

Textures Textures::instance = Textures();


