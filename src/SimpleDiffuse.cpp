#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class SimpleDiffuse : public Shader {
public:
  SimpleDiffuse();
  ~SimpleDiffuse();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SimpleDiffuse instance;
};

SimpleDiffuse::SimpleDiffuse()
  : Shader("Diffuse: Simple")
{
}

SimpleDiffuse::~SimpleDiffuse()
{
}

bool SimpleDiffuse::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f SH_DECL(ipos);
    ShInputNormal3f SH_DECL(inorm);
    
    ShOutputPosition4f SH_DECL(opos); // Position in NDC
    ShOutputNormal3f SH_DECL(onorm);
    ShInOutTexCoord2f SH_DECL(tc); // pass through tex coords
    ShOutputVector3f SH_DECL(lightv); // direction to light

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

    ShPoint3f SH_DECL(posv) = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    lightv = normalize(Globals::lightPos - posv); // Compute light direction
  } SH_END;
  
  ShColor3f SH_DECL(color) = ShColor3f(.2, 0.5, 0.9);
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputNormal3f SH_DECL(normal);
    ShInputTexCoord2f SH_DECL(tc); // ignore texcoords
    ShInputVector3f SH_DECL(light);
    ShInputPosition4f SH_DECL(posh);

    ShOutputColor3f SH_DECL(result);
    
    normal = normalize(normal);
    light = normalize(light);

    result = pos(normal | light) * color;
  } SH_END;
  return true;
}

SimpleDiffuse SimpleDiffuse::instance = SimpleDiffuse();


