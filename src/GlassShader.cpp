#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class GlassShader : public Shader {
public:
  ShProgram vsh, fsh;

  GlassShader();
  ~GlassShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

GlassShader::GlassShader()
  : Shader("Glass")
{
}

GlassShader::~GlassShader()
{
}

bool GlassShader::init()
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

  ShAttrib1f theta = ShAttrib1f(1.3f);
  theta.name("relative indices of refraction");
  theta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;  // view-space normal
    ShOutputVector3f reflv; // Compute reflection vector
    ShOutputVector3f refrv; // Compute refraction vector
    ShOutputAttrib1f fres; // Compute fresnel term

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f viewv = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector
    refrv = refract(viewv,onorm,theta); // Compute refraction vector
    fres = fresnel(viewv,onorm,theta); // Compute fresnel term

    // actually do reflection and refraction lookup in model space
    reflv = Globals::mv_inverse | reflv;
    refrv = Globals::mv_inverse | refrv;
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f n;  // normal
    ShInputVector3f reflv; // Compute reflection vector
    ShInputVector3f refrv; // Compute refraction vector
    ShInputAttrib1f fres; // Compute fresnel term

    ShOutputColor3f result;
    
    result = fres*cubemap(reflv)(0,1,2) + (1.0f-fres)*cubemap(refrv)(0,1,2); 
  } SH_END;

  return true;
}

void GlassShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

GlassShader the_glass_shader;
