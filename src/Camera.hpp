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
#ifndef SMASHTEST_CAMERA_HPP
#define SMASHTEST_CAMERA_HPP

#include <sh/sh.hpp>
#include <iostream>

class Camera {
public:
  Camera();

  void move(float x, float y, float z);
  void orbit(int sx, int sy, int x, int y, int w, int h);
  
  void glModelView();
  void glProjection(float aspect);

  SH::ShMatrix4x4f shModelView();
  SH::ShMatrix4x4f shModelViewProjection(SH::ShMatrix4x4f viewport);

private:
  SH::ShMatrix4x4f perspective(float fov, float aspect, float near, float far);

  SH::ShMatrix4x4f proj;
  SH::ShMatrix4x4f rots;
  SH::ShMatrix4x4f trans;

  friend std::ostream &operator<<(std::ostream &output, Camera &camera);
  friend std::istream &operator>>(std::istream &input, Camera &camera);
};

#endif
