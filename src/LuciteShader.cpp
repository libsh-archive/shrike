#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class LuciteShader : public Shader {
public:
  ShProgram vsh, fsh;

  LuciteShader();
  ~LuciteShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

LuciteShader::LuciteShader()
  : Shader("Refraction: Lucite")
{
}

LuciteShader::~LuciteShader()
{
}

bool LuciteShader::init()
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

  ShAttrib3f theta = ShAttrib3f(1.32f,1.3f,1.28f);
  theta.name("relative indices of refraction");
  theta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos;    // Position in NDC
    ShOutputNormal3f onorm;     // view-space normal
    ShOutputVector3f reflv;     // reflection vector
    ShOutputVector3f refrv[3];  // refraction vectors (per RGB channel)
    ShOutputAttrib3f fres;      // fresnel terms (per RGB channel)

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector

    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;

    for (int i=0; i<3; i++) {
    	refrv[i] = refract(viewv,onorm,theta[i]); // Compute refraction vectors

        // actually do refraction lookup in model space
        refrv[i] = Globals::mv_inverse | refrv[i];

        fres[i] = fresnel(viewv,onorm,theta[i]); // Compute fresnel term
    }
  } SH_END;
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f n;  // normal
    ShInputVector3f reflv;     // reflection vector
    ShInputVector3f refrv[3];     // refraction vectors (per RGB channel)
    ShInputAttrib3f fres;         // fresnel terms (per RGB channel)

    ShOutputColor3f result;
    
    result = fres*cubemap(reflv)(0,1,2);
    for (int i=0; i<3; i++) {
        result[i] += (1.0f-fres[i])*cubemap(refrv[i])(i); 
    }
  } SH_END;

  return true;
}

void LuciteShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

LuciteShader the_lucite_shader;
