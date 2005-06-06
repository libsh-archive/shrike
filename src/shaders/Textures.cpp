// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
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
  image.loadPng(SHMEDIA_DIR "/textures/abcd.png");

  // Here we could be using...
  // ShArray: nearest-neighbour lookup, no mipmapping
  // ShTable: linear interpolation, no mipmapping
  // ShTexture: linear interpolation and mipmapping
  ShTexture2D< ShColor3f > texture(image.width(), image.height());

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


