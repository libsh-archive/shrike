#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class EnvMapShader : public Shader {
public:
  ShProgram vsh, fsh;

  EnvMapShader();
  ~EnvMapShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

EnvMapShader::EnvMapShader()
  : Shader("Environment Map Mirror Shader")
{
}

EnvMapShader::~EnvMapShader()
{
}

bool EnvMapShader::init()
{
  std::cerr << "Initializing " << name() << std::endl;

  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShImage test_image;
  test_image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png");

  ShTextureCube<ShColor4f> cubemap(test_image.width(), test_image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
      cubemap.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputVector3f reflv; // reflection vector

    opos = Globals::mvp | ipos; // Compute NDC position
    ShNormal3f onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector
    reflv = reflect(viewv,onorm); // Compute reflection vector
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputVector3f reflv;

    ShOutputColor3f result;
    
    result = cubemap(reflv)(0,1,2); 
  } SH_END;

  return true;
}

void EnvMapShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

EnvMapShader the_envmap_shader;
