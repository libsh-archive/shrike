// multiple homomorphic materials blended with a material map
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class JeweledShader : public Shader {
public:
  JeweledShader();
  ~JeweledShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static JeweledShader instance;
};

JeweledShader::JeweledShader()
  : Shader("Jeweled")
{
}

JeweledShader::~JeweledShader()
{
}

bool JeweledShader::init()
{
  ShAttrib2f texture_scale(1.0,1.0);
  texture_scale.name("Texture Scale");
  texture_scale.range(0.0,10.0);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;// model-space position
    ShInputNormal3f inorm; // model-space normal
    ShInputTexCoord2f tc;  // texture coordinate (passed through)
    ShInputVector3f itan;  // model-space tangent
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputVector3f light;  // direction to light (surface frame)
    ShOutputVector3f view;   // direction to eye (surface frame)
    ShOutputTexCoord2f otc;  // tex coords passed through
    ShOutputVector3f reflv;  // reflection vector

    otc = tc * texture_scale;
    opos = Globals::mvp | ipos; // Compute NDC position
    ShNormal3f n = Globals::mv | inorm; // Compute view-space normal
    ShNormal3f t = Globals::mv | itan; // Compute view-space normal
    n = normalize(n);
    t = normalize(t);

    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f lightv = normalize(Globals::lightPos - posv); // Compute light direction
    ShVector3f viewv = -normalize(posv); // Compute view vector
    reflv = reflect(viewv,n);  // view-space reflection vector

    // compute local surface frame (in view space)
    // ShVector3f t = normalize(itan - (itan|n)*n);
    ShVector3f s = normalize(cross(t,n));
    
    // project view and light vectors onto local surface frame
    view(0) = (t|viewv);
    view(1) = (s|viewv);
    view(2) = (n|viewv);

    light(0) = (t|lightv);
    light(1) = (s|lightv);
    light(2) = (n|lightv);  // if positive, is irradiance scale
  } SH_END;

  ShImage image;

#define NMATS 3
  ShTexture2D<ShColor3f> ptex[NMATS];
  ShTexture2D<ShColor3f> qtex[NMATS];

  image.loadPng(SHMEDIA_DIR "/brdfs/satin/satinp.png");
  ptex[0].size(image.width(), image.height());
  ptex[0].memory(image.memory());
  // ptex[0].name("Satin p texture");

  image.loadPng(SHMEDIA_DIR "/brdfs/satin/satinq.png");
  qtex[0].size(image.width(), image.height());
  qtex[0].memory(image.memory());
  // ptex[0].name("Satin q texture");
  
  image.loadPng(SHMEDIA_DIR "/brdfs/mystique/mystique64_0.png");
  ptex[1].size(image.width(), image.height());
  ptex[1].memory(image.memory());
  // ptex[1].name("Mystique q texture");

  image.loadPng(SHMEDIA_DIR "/brdfs/mystique/mystique64_1.png");
  qtex[1].size(image.width(), image.height());
  qtex[1].memory(image.memory());

  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_0.png");
  ptex[2].size(image.width(), image.height());
  ptex[2].memory(image.memory());
  // ptex[1].name("Garnet red q texture");

  image.loadPng(SHMEDIA_DIR "/brdfs/garnetred/garnetred64_1.png");
  qtex[2].size(image.width(), image.height());
  qtex[2].memory(image.memory());

  // Specular highlight (to be added when needed...)
  image.loadPng(SHMEDIA_DIR "/brdfs/specular.png");
  ShTexture2D<ShColor3f> stex(image.width(), image.height());
  stex.memory(image.memory());
  // stex.name("Specular highlight texture");
  
  // Cube map for mirror reflection
  std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
  ShTextureCube<ShColor4f> env(image.width(), image.height());
  {
    for (int i = 0; i < 6; i++) {
      ShImage image;
      image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
      env.memory(image.memory(), static_cast<ShCubeDirection>(i));
    }
  }
  
  ShWrapRepeat< ShTexture2D<ShColor3f> > mat[NMATS];

  // Material map, channel 0
  image.loadPng(SHMEDIA_DIR "/mats/goldknot_ch1.png");
  // image.loadPng(SHMEDIA_DIR "/mats/colourknot_ch1.png");
  mat[0].size(image.width(), image.height());
  mat[0].memory(image.memory());
  // mat[0].name("Filgiree material map");
  
  // Material map, channel 1
  image.loadPng(SHMEDIA_DIR "/mats/colourknot_ch2_ch4.png");
  mat[1].size(image.width(), image.height());
  mat[1].memory(image.memory());
  // mat[1].name("Base material map");
  
  // Material map, channel 2
  image.loadPng(SHMEDIA_DIR "/mats/colourknot_ch3.png");
  mat[2].size(image.width(), image.height());
  mat[2].memory(image.memory());
  // mat[2].name("Knot interior material map");

  // Scale factors
  ShColor3f alpha[NMATS];
  ShAttrib1f diffuse[NMATS];
  ShAttrib1f specular[NMATS];
  ShAttrib1f mirror[NMATS];

  // for satin
  alpha[0] = ShColor3f(0.762367,0.762367,0.762367);
  alpha[0].name("Satin correction color");
  diffuse[0] = ShAttrib1f(1.0);
  diffuse[0].range(0.0,5.0);
  diffuse[0].name("Satin diffuse scale");
  specular[0] = ShAttrib1f(0.0);
  specular[0].range(0.0,1.0);
  specular[0].name("Satin specular scale");
  mirror[0] = ShAttrib1f(0.05);
  mirror[0].range(0.0,1.0);
  mirror[0].name("Satin mirror scale");

  // for mystique
  alpha[1] = ShColor3f(0.0011786,0.165452,0.0338959);
  alpha[1].name("Mystique correction color");
  diffuse[1] = ShAttrib1f(10.0);
  diffuse[1].range(0.0,20.0);
  diffuse[1].name("Mystique diffuse scale");
  specular[1] = ShAttrib1f(0.3);
  specular[1].range(0.0,1.0);
  specular[1].name("Mystique specular scale");
  mirror[1] = ShAttrib1f(0.07);
  mirror[1].range(0.0,1.0);
  mirror[1].name("Mystique mirror scale");

  // for garnet red
  alpha[2] = ShColor3f(0.0410592,0.0992037,0.0787714);
  alpha[2].name("Garnet red correction color");
  diffuse[2] = ShAttrib1f(5.0);
  diffuse[2].range(0.0,20.0);
  diffuse[2].name("Garnet red diffuse scale");
  specular[2] = ShAttrib1f(0.4);
  specular[2].range(0.0,1.0);
  specular[2].name("Garnet red specular scale");
  mirror[2] = ShAttrib1f(0.09);
  mirror[2].range(0.0,1.0);
  mirror[2].name("Garnet red mirror scale");

  // Light parameters
  ShColor3f light_color = ShColor3f(1.0,1.0,1.0);
  light_color.range(0.0,1.0);
  light_color.name("Light color");
  ShAttrib1f light_power = ShAttrib1f(5.0);
  light_power.range(0.0,20.0);
  light_power.name("Light power");
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputVector3f light;
    ShInputVector3f view;
    ShInputTexCoord2f u;
    ShInputVector3f reflv;  

    ShOutputColor3f result;

    // Normalize (theoretically not needed if compiler smart enough)
    light = normalize(light);
    view = normalize(view);

    // compute half vector
    ShVector3f half = normalize(view + light); 

    // Theoretically not needed if use common subexpression elimination...
    ShTexCoord2f hu = parabolic_norm(half);
    ShTexCoord2f lu = parabolic_norm(light);
    ShTexCoord2f vu = parabolic_norm(view);

    // Look up shared specular highlight component 
    ShColor3f spec = stex(hu);
    // Look up shared mirror reflection component 
    ShColor3f mirr = env(reflv)(0,1,2);

    // BUG: doesn't work (satin always white) if this not here...
    // in theory, should not be needed, bug in compiler?
    result = ShColor3f(0.0,0.0,0.0);
    
    // sum contributions of all materials
    for (int i=0; i<NMATS; i++) {
       ShColor3f f;

       // Incorporate diffuse scale, correction factor, and irradiance
       f = diffuse[i] * alpha[i] * pos(light(2));

       // do texture lookups
       f *= ptex[i](lu);
       f *= qtex[i](hu);
       f *= ptex[i](vu);

       // Add in specular highlight term (should use Fresnel here)
       f += specular[i] * spec * pos(light(2));
       // Add in mirror term (should use Fresnel here) 
       f += mirror[i] * mirr;

       // accumulate, masked by material map
       result += f * mat[i](u);
    }
    // Take into account light power and colour
    result *= light_power * light_color;
  } SH_END;
  return true;
}

JeweledShader JeweledShader::instance = JeweledShader();


