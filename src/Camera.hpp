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
