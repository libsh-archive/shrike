#ifndef GRMONITOR_HPP
#define GRMONITOR_HPP

#include <sh/sh.hpp>

class GrPort;

class GrMonitor {
public:
  GrMonitor(double x, double y);

  void setVertexProgram(const SH::ShProgram& program)
  {
    m_vp = program;
  }
  void setFragmentProgram(const SH::ShProgram& program)
  {
    m_fp = program;
  }

  void draw();
  
private:

  SH::ShProgram m_vp, m_fp;
  
  double m_x, m_y;
  double m_width, m_height;
};

#endif
