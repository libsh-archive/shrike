#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class HomomorphicShader : public Shader {
public:
  HomomorphicShader();
  ~HomomorphicShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static HomomorphicShader instance;
};

HomomorphicShader::HomomorphicShader()
  : Shader("Homomorphic Factorization: Parabolic")
{
}

HomomorphicShader::~HomomorphicShader()
{
}

bool HomomorphicShader::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputVector3f halfv;
    ShOutputVector3f lightv; // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction

    ShPoint3f viewv = -normalize(posv); // Compute view vector
    halfv = normalize(viewv + lightv); // Compute half vector
  } SH_END;

  ShImage image;

  // TODO: should have array of available BRDFs with correction
  // factor for each, hidden uniforms (don't want user to play with
  // alpha, really), pulldown menu to select BRDFs from list,
  // settings for extra specularities, etc. etc.
  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_0.png");
  ShTexture2D<ShColor3f> ptex(image.width(), image.height());
  ptex.memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_1.png");
  ShTexture2D<ShColor3f> qtex(image.width(), image.height());
  qtex.memory(image.memory());

  // these scale factors are specific to garnet red
  ShColor3f SH_DECL(alpha) = ShColor3f(1.0,1.0,1.0);
  ShAttrib1f SH_DECL(scale) = ShAttrib1f(1.0);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f normal;
    ShInputTexCoord2f tc; // ignore texcoords (for now)
    ShInputVector3f half;
    ShInputVector3f light;
    ShInputPosition4f posh;

    ShOutputColor3f result;

    // Normalize (theoretically not needed if compiler smart enough)
    light = normalize(light);
    normal = normalize(normal);
    half = normalize(half);
    
    // Incorporate scale, correction factor, and irradiance
    result = scale * alpha * irradiance(normal,light);

    // TODO: SHOULD use automatic projective normalization in texture lookup...
    // and/or parabolic texture type instead.
    result *= ptex(parabolic_norm(light));
    result *= qtex(parabolic_norm(half));
  } SH_END;
  return true;
}

// TODO: above code actually results in redundant normalization calls
// to light (it is normalized in both irradiance and in parabolic_norm).
// Rather than "fix" this code, this shader should be used as a test
// case (and an example) for automatic common subexpression elimination.
// Note that repeated normalization can be avoided with unit flags,
// so inserting the commented-out code block for normalization fixes
// the problem from an efficiency standpoint... even though vectors
// are normalized more than once in the code, the final assembly should
// only do it once.

HomomorphicShader HomomorphicShader::instance = HomomorphicShader();


