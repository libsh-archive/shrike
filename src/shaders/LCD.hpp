#ifndef LCD_HPP
#define LCD_HPP
#include <sh/sh.hpp>
#include "Shader.hpp"

class LCD : public Shader {
public:
  LCD();
  ~LCD();

  bool init();

  SH::ShProgram vertex() { return vsh;}
  SH::ShProgram fragment() { return fsh;}

  SH::ShProgram vsh, fsh;

  static LCD instance;
};

SH::ShAttrib1f lcd(const SH::ShTexCoord2f& tc, SH::ShAttrib1f number,
                   int intDigits = 3, int fracDigits = 1, bool showgrid = false, bool handleneg = true);

#endif
