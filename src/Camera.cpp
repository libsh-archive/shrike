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
#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "ShTrackball.hpp"
#include "Camera.hpp"

using namespace SH;

Camera::Camera()
{
  proj = perspective(45, 1, 1, 3000);
}

void printMatrix(std::ostream& out, const ShMatrix4x4f& mat)
{
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float v;
      mat[i](j).getValues(&v);
      out << v << (j == 3 ? '\n' : ' ');
    }
  }
}

std::ostream &operator<<(std::ostream &out, Camera &camera)
{
  printMatrix(out, camera.rots);
  printMatrix(out, camera.trans);
  return out;
}

std::istream &operator>>(std::istream &in, Camera &camera)
{
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      // TODO
      //      in >> camera.rots[i](j);
      //      in >> camera.trans[i](j);
    }
  }
  return in;
}

void Camera::glModelView()
{
  float values[16];
  for (int i = 0; i < 16; i++) trans[i%4](i/4).getValues(&values[i]);
  glMultMatrixf(values);
  for (int i = 0; i < 16; i++) rots[i%4](i/4).getValues(&values[i]);
  glMultMatrixf(values);
}

void Camera::glProjection(float aspect)
{
  proj = perspective(45, aspect, 1, 3000);
  float values[16];
  for (int i = 0; i < 16; i++) proj[i%4](i/4).getValues(&values[i]);
  glMultMatrixf(values);
}

ShMatrix4x4f Camera::shModelView()
{
  return (trans | rots);
}

ShMatrix4x4f Camera::shModelViewProjection(ShMatrix4x4f viewport)
{
  return (viewport | (proj | (trans | rots)));
}

void Camera::move(float x, float y, float z)
{
  ShMatrix4x4f m;
  m[0](3) = x;
  m[1](3) = y;
  m[2](3) = z;

  trans = (m | trans);
}

void Camera::orbit(int sx, int sy, int x, int y, int w, int h)
{
  ShTrackball t;
  t.resize(w, h);
  rots = (t.rotate(sx, sy, x, y) | rots);
}

//-------------------------------------------------------------------
// perspective 
//-------------------------------------------------------------------
ShMatrix4x4f Camera::perspective(float fov, float aspect, float near_, float far_)
{
  float zmin = near_;
  float zmax = far_;
  float ymax = zmin*tan(fov*(M_PI/360));
  float ymin = -ymax;
  float xmin = ymin*aspect;
  float xmax = ymax*aspect;

  
  
  ShMatrix4x4f ret;

  ret[0](0) = 2.0*zmin/(xmax-xmin);
  ret[0](1) = 0.0;
  ret[0](2) = 0.0;
  ret[0](3) = 0.0;

  ret[1](0) = 0.0;
  ret[1](1) = 2.0*zmin/(ymax-ymin);
  ret[1](2) = 0.0;
  ret[1](3) = 0.0;

  ret[2](0) = 0.0;
  ret[2](1) = 0.0;
  ret[2](2) = -(zmax+zmin)/(zmax-zmin);
  ret[2](3) = -2.0*zmax*zmin/(zmax-zmin);

  ret[3](0) = 0.0;
  ret[3](1) = 0.0;
  ret[3](2) = -1.0;
  ret[3](3) = 0.0;

  return ret;
}
