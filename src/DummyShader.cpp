#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class DummyShader : public Shader {
public:
  ShProgram vsh, fsh;

  DummyShader();
  ~DummyShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
};

DummyShader::DummyShader()
  : Shader("Dummy Shader")
{
}

DummyShader::~DummyShader()
{
}

bool DummyShader::init()
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

  ShImage input_image;
  input_image.loadPng(SHMEDIA_DIR "/bumpmaps/bumps.png");
  int w = input_image.width();
  int h = input_image.height();
  ShImage output_image(w,h,3);
  for (int j = 0; j < h; j++) {	  
    int jp1 = j + 1;
    if (jp1 >= h) jp1 = 0;
    int jm1 = (j - 1);	  
    if (jm1 < 0) jm1 = h - 1;
    for (int i = 0; i < w; i++) {
      int ip1 = i + 1;
      if (ip1 >= w) ip1 = 0;
      int im1 = (i - 1);	  
      if (im1 < 0) im1 = w - 1;
      float x, y, z;
      x = (input_image(ip1,j,0) - input_image(im1,j,0))/2.0f; 
      output_image(i,j,0) = x/2.0f + 0.5f; 
      y = (input_image(i,jp1,0) - input_image(i,jm1,0))/2.0f; 
      output_image(i,j,1) = y/2.0f + 0.5f; 
      z = x*x + y*y;
      z = (z > 1.0f) ? z = 0.0f : sqrt(1 - z);
      output_image(i,j,2) = z;    
    }
  }
  output_image.savePng(SHMEDIA_DIR "/bumpmaps/bumps_normals.png");

  ShTexture2D<ShVector3f> bump(w,h);
  bump.memory(output_image.memory());
  
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
    ShVector3f b = bump(u);
    ShVector3f bn = t * b(0) + s * b(1) + n * b(2);
    ShVector3f r = reflect(v,bn); // Compute reflection vector
    result = cubemap(r)(0,1,2); 
  } SH_END;

  return true;
}

void DummyShader::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

DummyShader the_dummy_shader;
