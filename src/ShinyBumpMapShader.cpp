#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class ShinyBumpMapShader : public Shader {
public:
  ShProgram vsh, fsh;

  ShinyBumpMapShader();
  ~ShinyBumpMapShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

ShinyBumpMapShader::ShinyBumpMapShader()
  : Shader("Shiny Bump Map Shader")
{
}

ShinyBumpMapShader::~ShinyBumpMapShader()
{
}

bool ShinyBumpMapShader::init()
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
    ShInputVector3f itan;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc;    // texture coordinates
    ShOutputNormal3f onorm; // view-space normal
    ShOutputVector3f otan; // view-space tangent
    ShOutputVector3f viewv;  // view vector

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    otan = Globals::mv | itan; // Compute view-space tangent
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    viewv = -ShVector3f(posv); // Compute view vector
  } SH_END;

  ShImage image;
  image.loadPng(SHMEDIA_DIR "/bumpmaps/bumps_normals.png");
  ShTexture2D<ShVector3f> bump(image.width(),image.height());
  bump.memory(image.memory());

  ShAttrib3f SH_DECL(scale) = ShAttrib3f(2.0,2.0,1.0);
  scale.range(0.0f,10.0f);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f n;
    ShInputVector3f t;
    ShInputVector3f v;

    ShOutputColor3f result;
    
    ShVector3f s = cross(t,n);
    t = normalize(t);
    s = normalize(s);
    n = normalize(n);
    ShVector3f b = bump(u) - ShAttrib3f(0.5,0.5,0.0);
    b *= scale;
    ShVector3f bn = t * b(0) + s * b(1) + n * b(2);
    ShVector3f r = reflect(v,bn); // Compute reflection vector
    result = cubemap(r)(0,1,2); 
  } SH_END;

  return true;
}

void ShinyBumpMapShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

ShinyBumpMapShader the_shinybumpmap_shader;
