#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

class Ashikhmin : public Shader {
public:
  Ashikhmin();
  ~Ashikhmin();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;
};

Ashikhmin::Ashikhmin()
  : Shader("Basic Lighting Models: Ashikhmin")
{
}

Ashikhmin::~Ashikhmin()
{
}

template<int N>
ShGeneric<N, float> pow5(const ShGeneric<N, float>& f)
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

#define CLAMP(x) max(x, 0.01)
  
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

bool Ashikhmin::init()
{
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  ShProgram keeper = SH_BEGIN_PROGRAM() {
    ShInputVector3f itan;
    ShOutputVector3f otan = Globals::mv | itan;
  } SH_END_PROGRAM;
  vsh = (shSwizzle("normal", "viewVec", "halfVec", "lightVec", "posh") << vsh) & keeper;

  ShColor3f SH_DECL(diffuse) = ShColor3f(0.0, 1.0, 0.5);
  ShColor3f SH_DECL(specular) = ShColor3f(1.0, .5, 0.8)/20.0f;
  ShColor3f SH_DECL(ambient) = ShColor3f(0.0, 0.1, 0.05);
  ShAttrib1f SH_DECL(nu) = 1000.0f;
  ShAttrib1f SH_DECL(nv) = 10.0f;
  specular.range(0.0f, 0.05f);
  nu.range(10.0f, 10000.0f);
  nv.range(10.0f, 10000.0f);
  
  fsh = SH_BEGIN_FRAGMENT_PROGRAM {
    ShInputNormal3f normal;
    ShInputVector3f viewvec;
    ShInputVector3f halfvec;
    ShInputVector3f lightvec;
    ShInputVector3f tan1;
    ShInputPosition3f pos;

    ShOutputColor3f color;

    tan1 = normalize(tan1);
    normal = normalize(normal);

    ShVector3f tan2 = cross(normal, tan1);
    color = ashikhmin(nu, nv, normalize(normal), normalize(halfvec), normalize(lightvec),
                      normalize(viewvec),
                      tan1, tan2,
                      specular, diffuse)
            + ambient;

  } SH_END_PROGRAM;
    
  return true;
}

Ashikhmin ash;

