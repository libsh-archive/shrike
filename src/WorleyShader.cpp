#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class WorleyShader : public Shader {
public:
  WorleyShader(bool tex, int which);
  ~WorleyShader();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  ShAttrib4f getWorley(int i, ShTexCoord2f t, ShAttrib1f p);

  ShUtil::ShWorley worleyizer;
  ShUtil::ShWorleyMetric metric;
  int whichW;
  bool useTexture;
};

#define WORLEY_FREQ 8

WorleyShader::WorleyShader(bool tex, int which)
  : Shader("Worley textures"),
    worleyizer(WORLEY_FREQ, tex), whichW(which), useTexture(tex)
{
}

WorleyShader::~WorleyShader()
{
}

ShAttrib4f WorleyShader::getWorley(int i, ShTexCoord2f t, ShAttrib1f p)
{
  ShAttrib4f w, z;
  ShAttrib4f wC;
  switch (i) {
  case 1:
    wC = ShAttrib4f(1,0,0,0);
    w(2) = worleyizer.worleyNoGradient(t, wC, L2_SQ);
    w(2) = (ShAttrib1f(1)-w(2) - p)*ShAttrib1f(30);
    w(2) = clamp(0.0f, 1.0f, w(2));
    w(0,1) = w(2,2);
    break;
  case 2:
    wC = ShAttrib4f (-1,1.2,0,0);
    w(0,1,2) = worleyizer.worleyNoGradient(t, wC, L1)(0,0,0);
    wC = ShAttrib4f (0,1,1,0);
    z(0,1,2) = worleyizer.worleyNoGradient(t*ShAttrib1f(2), wC, L2_SQ)(0,0,0);
    w = w-z;
    break;
  case 3:
    wC = ShAttrib4f (-1,1,0,0);
    w(2) = worleyizer.worleyNoGradient(t, wC, L1);
    w(2) = (ShAttrib1f(1)-w(2) - p)*ShAttrib1f(30);
    //w(2) = clamp(0.0f, 1.0f, w(2));
      
    wC = ShAttrib4f (0,-1,1,0);
    w(1) = worleyizer.worleyNoGradient(t, wC, L1);
    w(1) = (ShAttrib1f(1)-w(1) - p)*ShAttrib1f(10);
    //w(1) = clamp(0.0f, 1.0f, w(1));

    w(2) = clamp(0.0f, 1.0f, w(1)+w(2));
    w(0,1) = w(2,2);
    break;
  case 4:
    wC = ShAttrib4f (0,0,0,1);
    w(2) = worleyizer.worleyNoGradient(t, wC, L1) * ShAttrib1f(0.5);
    w(2) = (ShAttrib1f(1)-w(2) - p)*ShAttrib1f(30);
    w(2) = clamp(0.0f, 1.0f, w(2));
      
    w(0) = worleyizer.worleyNoGradient(t*ShAttrib1f(2), wC, L1) * ShAttrib1f(0.5);
    w(0) = (ShAttrib1f(1)-w(0) - p)*ShAttrib1f(30);
    w(0) = clamp(0.0f, 1.0f, w(0));
    w(1) = ShAttrib1f(1) - w(0);
    break;
  case 0:
  default:
    wC = ShAttrib4f(1,0,0,0);
    w(0,1,2) = worleyizer.worley(t*ShAttrib1f(2), wC, L2_SQ);
    break;
  }
  w(3) = ShAttrib1f(1) - w(2);
  return w;
}

bool WorleyShader::init()
{
  vsh = SH_BEGIN_VERTEX_PROGRAM {
    ShInputNormal3f normal;
    ShInputTexCoord2f texture;
    ShInputPosition4f p;

    ShOutputVector3f ohv;
    ShOutputNormal3f on;
    ShOutputTexCoord2f ot;
    ShOutputAttrib1f ec;	//irradiance
    ShOutputPosition4f opd;

    //Used for lighting
    ShNormal3f nvt = normalize(Globals::mv | normal);
    ShPoint3f pvt = (Globals::mv | p)(0,1,2);
    ShVector3f vvv = -normalize(pvt);
    ShVector3f lvv = normalize(Globals::lightPos - pvt);
    ShAttrib1f ect = (nvt | lvv);
    
    opd = Globals::mvp | p;
    on = nvt;
    ohv = normalize(lvv + vvv);
    ot = texture;
    ec = ect * (ect>0.0);
    
  } SH_END_PROGRAM;

  ShAttrib1f SH_DECL(param) = .5;
  param.range(0.0f, 1.0f);
  ShAttrib1f SH_DECL(exponent) = 35.0;
  exponent.range(10.0f, 500.0f);
  ShAttrib1f SH_DECL(toggle) = 0.0;
  toggle.range(0.0f, 1.0f);
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputVector3f hv;
    ShInputNormal3f n;
    ShInputTexCoord2f t;
    ShInputAttrib1f ec;
    ShInputPosition4f p;
    
    ShOutputColor3f out;

    ShAttrib4f w = getWorley(whichW, t, param);

    ShNormal3f newn = n;
    if (whichW==0)
    {
      ShNormal2f offset = normalize(w(1,2)*ShAttrib1f(0.25)) * param;
      newn(0)+= offset(0);
      newn(1)+= offset(1);
      newn = normalize(newn);
      w(2) = w(0);
      w(3) = ShAttrib1f(1) - w(2);
    }
    else
    newn = n;

    ShAttrib1f spec;
    spec = dot(normalize(hv), normalize(newn)) ^ exponent;

    ShColor3f col1;
    ShColor3f col2;
    
    switch (whichW)
    {
    case 0:
      col1 = ShColor3f(0.7,0.2,0.3)*w(3) + ShColor3f(0.4,0,0)*w(2);
      //col1 = w(2,3,2);
      spec*= w(3);
      break;
    case 1:
      col1 = ShColor3f(0.27,0.35,0.45)*w(3) + ShColor3f(1,0.7,0)*w(2);
      break;
    case 2:
      col1 = ShColor3f(0.5,0,0)*w(3) + ShColor3f(0,0,1)*w(2);
      break;
    case 3:
      col1 = ShColor3f(0.45,0.3,0)*w(3) + ShColor3f(1,0.95,0.8)*w(2);
      spec*= 0.2;
      break;
    case 4:
      //col1 = ShColor3f(0,0.45,0)*w(0) + ShColor3f(0,0.3,0.8)*w(2);
      col1 = ShColor3f(0,0.2,0.8)*w(0) + ShColor3f(0.6,0.6,0.7)*w(2);
      spec*= col1(1);
      break;
    case 5:
      col2(0) = (w(0)>ShAttrib1f(0));
      col1 = (ShColor3f(0,0.6,0)*w(3) + ShColor3f(0.5,0.5,0.5)*w(2))*(ShAttrib1f(1)-col2(0)) + col2(0)*(ShColor3f(0.2,0.2,0.9)*w(0));
      break;
    }
    col1 = (col1 + ShColor3f(1,1,1)*spec) * ec;
    col2 = w(1,0,1);

    out = toggle*col2 + (1.0-toggle)*col1;
  } SH_END_PROGRAM;
  return true;
}

void WorleyShader::bind()
{
  shBindShader(vsh);
  shBindShader(fsh);
}

WorleyShader worleyShader0( false, 0 );
WorleyShader worleyShader1( false, 1 );
WorleyShader worleyShader2( false, 2 );
WorleyShader worleyShader3( false, 3 );
WorleyShader worleyShader4( false, 4 );

