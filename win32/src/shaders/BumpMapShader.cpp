#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class BumpMapShader : public Shader
  {
  public:
    BumpMapShader();
    ~BumpMapShader();
    
    bool init();
    
    ShProgram vertex() { return vsh; }
    ShProgram fragment() { return fsh; }

  public:
    ShProgram vsh;
    ShProgram fsh;

    static BumpMapShader instance;
  };

BumpMapShader::BumpMapShader() :
  Shader("Bump and Frame Mapping: Diffuse Bump Mapping")
  {
  }

BumpMapShader::~BumpMapShader()
  {
  }

bool BumpMapShader::init()
  {
  std::cerr << "Initializing " << name() << std::endl;

  // stock vertex shader, feed in global light, swizzle
  // out texcoord for bumpmap, normal/tanget to build
  // local frame and lightVec for diffuse lighting
  vsh = ShKernelLib::shVsh(Globals::mv, Globals::mvp, 1, 1);
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  vsh = (shSwizzle("texcoord", "normal", "tangent", "lightVec", "posh") << vsh);

  ShImage image;
  image.loadPng(SHMEDIA_DIR "/bumpmaps/bumps_normals.png");
  ShTexture2D<ShVector3f> bump(image.width(),image.height());
  bump.memory(image.memory());

  ShAttrib1f SH_DECL(scale) = ShAttrib1f(1.0);
  scale.range(0.0f, 10.0f);
  ShColor3f SH_DECL(diffuse) = ShColor3f(0.5, 0.5, 0.5);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f iposh;
    ShInputTexCoord2f itex;
    ShInputNormal3f inrm;
    ShInputVector3f itan;
    ShInputVector3f ilv;

    ShOutputColor3f result;
    
    // generate bi-tangent and normalize local frame
    ShVector3f btan = cross(itan, inrm);
    itan = normalize(itan);
    btan = normalize(btan);
    inrm = normalize(inrm);

    // lookup perturbed normal, scale [0,1] -> [-1, 1],
    // apply scale factor, renormalize and tranformat
    ShVector3f nrm = 2.0f*bump(itex) - 1.0f;
    nrm(2) = scale*nrm(2);
    nrm = normalize(nrm);
    nrm = itan*nrm(0) + btan*nrm(1) + inrm*nrm(2);

    // perform standard diffuse lighting
    result = diffuse*dot(nrm, ilv);
  } SH_END;

  return true;
  }

BumpMapShader BumpMapShader::instance = BumpMapShader();
