#ifndef LCDSmall_HPP
#define LCDSmall_HPP
#include <sh/sh.hpp>
#include "Shader.hpp"

class LCDSmall : public Shader {
public:
  LCDSmall();
  ~LCDSmall();

  bool init();

  SH::ShProgram vertex() { return vsh;}
  SH::ShProgram fragment() { return fsh;}

  SH::ShProgram vsh, fsh;

  static LCDSmall instance;
};

SH::ShAttrib1f lcdSmall(const SH::ShTexCoord2f& tc, SH::ShAttrib1f number,
                   int intDigits = 3, int fracDigits = 1, bool showgrid = false, 
                   bool handleneg = true, float w = 0.2, float h = 0.5, float t = 0.02);

#endif
