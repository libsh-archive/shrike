#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

//#define IMG_TEXTURE 1

namespace {
struct FrameMap {
  FrameMap(int width, int height) : 
    normalMap(width, height, 3), tangentMap(width, height, 3) {}
    
  ShImage normalMap;
  ShImage tangentMap;
};

class FrameMapping : public Shader {
public:
  FrameMapping();
  ~FrameMapping();

  bool init();
  void bind();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  FrameMap genFrameMap(const ShImage& img, int& width, int& height);

  ShProgram vsh, fsh;
};

FrameMapping::FrameMapping()
  : Shader("FrameMapping lighting model")
{
}

FrameMapping::~FrameMapping()
{
}

template<int N>
ShVariableN<N, float> pow5(const ShVariableN<N, float>& f)
{
  ShAttrib<N, SH_TEMP, float> t =  f * f;
  return t * t * f;
}

ShColor3f schlick(ShColor3f refl, ShAttrib1f kh)
{
  return refl + (ShColor3f(1.0, 1.0, 1.0) - refl)*pow5(1.0f - kh);
}

ShColor3f ashikhmin_specular(ShAttrib1f nu, ShAttrib1f nv,
                             ShNormal3f n, ShVector3f h,
                             ShVector3f light, ShVector3f viewer,
                             ShVector3f u, ShVector3f v,
                             ShColor3f refl)
{
  ShVector3f k = viewer; // either light or viewer works here

#define CLAMP(x) min(x, 0.99)
  
  ShAttrib1f hn = CLAMP(h|n);
  ShAttrib1f kn = CLAMP(k|n);
  ShAttrib1f ln = CLAMP(light|n);
  ShAttrib1f vn = CLAMP(viewer|n);
  ShAttrib1f kh = CLAMP(k|h);
  ShAttrib1f hu = (h|u);
  ShAttrib1f hv = (h|v);
  
  ShAttrib1f scale = sqrt((nu + 1.0f) * (nv + 1.0f))/(8.0*M_PI);
  ShAttrib1f exponent = (nu*hu*hu + nv*hv*hv)/(1.0f - hn*hn);
  ShAttrib1f geom = pow(hn, exponent)/(kn*max(ln, vn));

  return scale * geom * schlick(refl, kh);
}

ShColor3f ashikhmin_diffuse(ShNormal3f normal,
                            ShVector3f light, ShVector3f viewer,
                            ShColor3f spec, ShColor3f diffuse)
{
  ShColor3f scale = (28.0/(23.0*M_PI))*diffuse*(ShColor3f(1.0, 1.0, 1.0) - spec);

  ShAttrib1f v = 1.0f - pow5(1.0f - max(normal|light, 0.0)/2.0f);
  ShAttrib1f l = 1.0f - pow5(1.0f - max(normal|viewer, 0.0)/2.0f);

  return scale * v * l;
}

ShColor3f ashikhmin(ShAttrib1f nu, ShAttrib1f nv,
                    ShNormal3f n, ShVector3f h,
                    ShVector3f light, ShVector3f viewer,
                    ShVector3f u, ShVector3f v,
                    ShColor3f spec, ShColor3f diffuse)
{
  return ashikhmin_specular(nu, nv, n, h, light, viewer, u, v, spec)
       + ashikhmin_diffuse(n, light, viewer, spec, diffuse);
}

int truncPow2(int a) 
{
  int exp = -1;
  while (a > 0) 
  {
    exp++;
    a /= 2;
  }
  return (1 << exp);
}

FrameMap FrameMapping::genFrameMap(const ShImage& img, int& width, int& height)
{
  int w =  truncPow2(img.width());
  int h = truncPow2(img.height());
  int side = ( w > h ) ? h : w;
  FrameMap result(side, side);
  float dx, dy;
  float factor = 1.0/360.6;
  for (int i = 0; i < side; i++)
  {
    for (int j = 0; j < side; j++)
    {
      if (j == 0) 
      {
        dx = 0.5*((img(j, i, 0) + img(j, i, 1) + img(j, i, 2))/3.0 - 
            (img(j+1, i, 0) + img(j+1, i, 1) + img(j+1, i, 2))/3.0);
      }
      else if (j == img.width()-1) 
      {
        dx = 0.5*((img(j-1, i, 0) + img(j-1, i, 1) + img(j-1, i, 2))/3.0 - 
            (img(j, i, 0) + img(j, i, 1) + img(j, i, 2))/3.0);
      }
      else 
      {
        dx = ((img(j-1, i, 0) + img(j-1, i, 1) + img(j-1, i, 2))/3.0 - 
            (img(j+1, i, 0) + img(j+1, i, 1) + img(j+1, i, 2))/3.0);
      }
      if (i == 0) 
      {
        dy = 0.5*((img(j, i, 0) + img(j, i, 1) + img(j, i, 2))/3.0 - 
            (img(j, i+1, 0) + img(j, i+1, 1) + img(j, i+1, 2))/3.0);
      }
      else if (i == img.height()-1) 
      {
        dy = 0.5*((img(j, i-1, 0) + img(j, i-1, 1) + img(j, i-1, 2))/3.0 - 
            (img(j, i, 0) + img(j, i, 1) + img(j, i, 2))/3.0);
      }
      else 
      {
        dy = ((img(j, i-1, 0) + img(j, i-1, 1) + img(j, i-1, 2))/3.0 - 
            (img(j, i+1, 0) + img(j, i+1, 1) + img(j, i+1, 2))/3.0);
      }
      ShVector3f gradient = factor*ShVector3f(dx, 0, -dy);
      ShVector3f normal = ShVector3f(0, 1, 0) + gradient;
      normal = normalize(normal);
      ShVector3f tangent = normalize(cross(cross(gradient, normal), normal));
      float values[3];
      normal.getValues(values);
      result.normalMap(j, i, 0) = values[0];
      result.normalMap(j, i, 1) = values[1];
      result.normalMap(j, i, 2) = values[2];
      tangent.getValues(values);
      result.tangentMap(j, i, 0) = values[0];
      result.tangentMap(j, i, 1) = values[1];
      result.tangentMap(j, i, 2) = values[2];
    }
  }
  width = side;
  height = side;
  return result;
}

bool FrameMapping::init()
{
#if (IMG_TEXTURE)
  ShImage texImg;
  texImg.loadPng(SHMEDIA_DIR "/textures/rustkd.png");
  int w, h;
  FrameMap fmap = genFrameMap(texImg, w, h);
  ShTexture2D<ShVector3f> normalMap(w, h);
  ShTexture2D<ShVector3f> tangentMap(w, h);
  normalMap.memory(fmap.normalMap.memory());
  tangentMap.memory(fmap.tangentMap.memory());
#endif
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  ShProgram keeper = SH_BEGIN_PROGRAM() {
    ShInputVector3f itan;
    ShOutputVector3f otan = Globals::mv | itan;
  } SH_END_PROGRAM;
  vsh = (shRange("texcoord")("normal", "posh") << vsh) & keeper;

  ShColor3f SH_DECL(diffuse1) = ShColor3f(1.0, 0.0, 0.0);
  ShColor3f SH_DECL(diffuse2) = ShColor3f(1.0, 1.0, 0.0);
  ShColor3f SH_DECL(specular) = ShColor3f(1.0, 1.0, 1.0)/20.0f;
  ShColor3f SH_DECL(ambient) = ShColor3f(0.1, 0.1, 0.1);
  ShAttrib1f SH_DECL(nu) = 1000.0f;
  ShAttrib1f SH_DECL(nv) = 10.0f;
  ShAttrib1f SH_DECL(fu) = 10.0f;
  ShAttrib1f SH_DECL(fv) = 10.0f;
  ShAttrib1f SH_DECL(angle) = M_PI/2.0;
  specular.range(0.0f, 0.05f);
  nu.range(10.0f, 10000.0f);
  nv.range(10.0f, 10000.0f);
  fu.range(1.0f, 100.0f);
  fv.range(1.0f, 100.0f);
  angle.range(0.0f, M_PI);
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputTexCoord2f texcoord;
    ShInputNormal3f normal;
    ShInputVector3f viewvec;
    ShInputVector3f halfvec;
    ShInputVector3f lightvec;
    ShInputVector3f tan1;
    ShInputPosition3f pos;

    ShOutputColor3f color;
#ifndef IMG_TEXTURE
    ShQuaternionf rot(angle, ShVector3f(normal));
    ShAttrib1f frac0 = frac(fu*texcoord(0));
    ShAttrib1f frac1 = frac(fv*texcoord(1));
    ShAttrib1f cond1 = (frac0 > 0.5);
    ShAttrib1f cond2 = (frac1 > 0.5);
    frac0 = frac(frac0*2.0);
    frac1 = frac(frac1*2.0);
    ShQuaternionf q = rot.inverse()*tan1*rot;
    ShAttrib1f cond12 = cond1*cond2 + (cond1==0.0)*(cond2==0.0);;
    tan1 = cond12*q.getVector()(1, 2, 3) + (cond12 == 0.0)*tan1;
    
    ShAttrib1f bumpAngle =(cond12*(frac1 - 0.5) + 
      (cond12==0.0)*(frac0 - 0.5));
    ShQuaternionf bump(-bumpAngle, ShVector3f(tan1));
    ShQuaternionf qn = bump.inverse()*normal*bump;
    normal = qn.getVector()(1,2,3);
    qn = bump.inverse()*tan1*bump;
    tan1 = qn.getVector()(1,2,3);
    ShColor3f diffuse = cond12*diffuse1 + (cond12 == 0.0)*diffuse2;
#else
    ShColor3f diffuse = diffuse1;
    ShVector3f ymap = normalMap(texcoord);
    ShVector3f zmap = tangentMap(texcoord);
    ShVector3f xmap = cross(ymap, zmap);
    ShMatrix4x4f rot;
    rot[0](0) = xmap(0);
    rot[1](0) = xmap(1);
    rot[2](0) = xmap(2);
    rot[0](1) = ymap(0);
    rot[1](1) = ymap(1);
    rot[2](1) = ymap(2);
    rot[0](2) = zmap(0);
    rot[1](2) = zmap(1);
    rot[2](2) = zmap(2);

    normal = rot | normal;
    tan1 = rot | tan1;
#endif
    ShVector3f tan2 = cross(normal, tan1);
    
    color = ashikhmin(nu, nv, normalize(normal), normalize(halfvec), normalize(lightvec),
                      normalize(viewvec),
                      tan1, tan2,
                      specular, diffuse)
            + ambient;

  } SH_END_PROGRAM;
    
  return true;
}

void FrameMapping::bind()
{
  vsh->code()->print(std::cerr);
  fsh->code()->print(std::cerr);
  std::cerr << "Binding " << name() << std::endl;
  shBindShader(vsh);
  shBindShader(fsh);
}

FrameMapping ash;
}

