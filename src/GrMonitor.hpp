#ifndef GRMONITOR_HPP
#define GRMONITOR_HPP

#include <sh/sh.hpp>

class GrPort;

class GrMonitor {
public:
  GrMonitor(GrPort* port);

  void setVertexProgram(const SH::ShProgram& program)
  {
    m_vp = program;
  }
  void setFragmentProgram(const SH::ShProgram& program)
  {
    m_fp = program;
  }

  void draw();
  void pick();

  double x() const { return m_x; }
  double y() const { return m_y; }
  
  void moveBy(double dx, double dy) { m_x += dx; m_y += dy; }
  void moveTo(double x, double y) { m_x = x; m_y = y; }

  GrPort* port() const { return m_port; }
  
private:

  GrPort* m_port;

  double m_x, m_y;
  double m_width, m_height;

  int m_gl_name;

  SH::ShProgram m_vp, m_fp;
};

void destroy(GrMonitor* monitor);

#endif
