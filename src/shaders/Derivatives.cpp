#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Derivatives : public Shader {
public:
  Derivatives();
  ~Derivatives();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  
  ShProgram vsh, fsh;

  static Derivatives instance;
};

Derivatives::Derivatives()
  : Shader("Derivatives: Derivatives")
{
}

Derivatives::~Derivatives()
{
}

bool Derivatives::init()
{
  std::cerr << "Initializing " << name() << std::endl;
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = shSwizzle("texcoord", "posh") << vsh;

  ShAttrib1f SH_DECL(scale_x) = 1.0;
  scale_x.range(0.0, 10.0);
  ShAttrib1f SH_DECL(scale_y) = 0.0;
  scale_y.range(0.0, 10.0);

  ShImage image;
  image.loadPng(SHMEDIA_DIR "/textures/rgby.png");
  ShTexture2D< ShColor3f > texture(image.width(), image.height());
  texture.memory(image.memory());
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f i;
    ShOutputColor3f o;

    ShColor3f t = texture(i);
    
    o = abs(scale_x * dx(t)) + abs(scale_y * dy(t));
  } SH_END_PROGRAM;
  return true;
}


Derivatives Derivatives::instance = Derivatives();
