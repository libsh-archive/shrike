// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#define IMG_TEXTURE 1
#define USE_QUAT 0 

namespace {
struct FrameMap {
  FrameMap() {};
  FrameMap(int width, int height) :
#if (USE_QUAT)
  quaternionMap(width, height, 4)
#else
  normalMap(width, height, 3), 
  tangentMap(width, height, 3) 
#endif
  {}

#if (USE_QUAT)
  ShImage quaternionMap;
#else
  ShImage normalMap;
  ShImage tangentMap;
#endif
};

class FrameMapping : public Shader {
public:
  FrameMapping();
  ~FrameMapping();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}
  void mapImage(ShImage& image);

  FrameMap genFrameMap(const ShImage& img, int& width, int& height);
  
  ShProgram vsh, fsh;
};

FrameMapping::FrameMapping()
  : Shader("Bump and Frame Mapping: Anisotropic Frame Mapping")
{
}

FrameMapping::~FrameMapping()
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

void FrameMapping::mapImage(ShImage& image) 
{
  for (int i = 0; i < image.height(); i++) 
  {
    for (int j = 0; j < image.width(); j++)
    {
      for (int k = 0; k < image.elements(); k++) 
      {
        image(j, i, k) = 2.0 * image(j, i, k) - 1.0;
      }
    }
  }
}


bool FrameMapping::init()
{
  //ShEnvironment::optimizationLevel = 0;
#if (IMG_TEXTURE)
  FrameMap fmap;
  
  ShAttrib1f SH_DECL(mapNormal) = 1.0f;
  mapNormal.range(0.0f, 1.0f);
  ShAttrib1f SH_DECL(mapTangent) = 1.0f;
  mapTangent.range(0.0f, 1.0f);
#if (USE_QUAT)
  int w, h;
  fmap.quaternionMap.loadPng(SHMEDIA_DIR "/bumpmaps/frames.png");
  w = fmap.quaternionMap.width();
  h = fmap.quaternionMap.height();
  mapImage(fmap.quaternionMap);
  ShUnclamped< ShTextureRect<ShVector4f> > quaternionMap(w, h);
  quaternionMap.memory(fmap.quaternionMap.memory());
#else
  int w, h, w2, h2;
  fmap.normalMap.loadPng(SHMEDIA_DIR "/bumpmaps/frame_normals.png");
  fmap.tangentMap.loadPng(SHMEDIA_DIR "/bumpmaps/frame_tangents.png");
  w = fmap.normalMap.width();
  h = fmap.normalMap.height();
  w2 = fmap.tangentMap.width();
  h2 = fmap.tangentMap.height();
  mapImage(fmap.normalMap);
  mapImage(fmap.tangentMap);
  ShUnclamped< ShTextureRect<ShVector3f> > normalMap(w, h);
  ShUnclamped< ShTextureRect<ShVector3f> > tangentMap(w2, h2);
	normalMap.memory(fmap.normalMap.memory());
  tangentMap.memory(fmap.tangentMap.memory());
#endif
#endif
  vsh = ShKernelLib::shVsh( Globals::mv, Globals::mvp );
  vsh = vsh << shExtract("lightPos") << Globals::lightPos;
  ShProgram keeper = SH_BEGIN_PROGRAM() {
    ShInputVector3f itan;
    ShOutputVector3f otan = Globals::mv | itan;
  } SH_END_PROGRAM;
  vsh = (shSwizzle("texcoord", "normal", "viewVec",
        "halfVec", "lightVec", "posh") << vsh) & keeper;

  ShColor3f SH_DECL(diffuse1) = ShColor3f(0.5, 0.5, 0.5);
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
#if (!IMG_TEXTURE)
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
		
    ShAttrib1f bumpAngle =(cond12*(frac1 - 0.5) + (cond12==0.0)*(frac0 - 0.5));
    ShQuaternionf bump(-bumpAngle, ShVector3f(tan1));
    ShQuaternionf qn = bump.inverse()*normal*bump;
    normal = qn.getVector()(1,2,3);
    qn = bump.inverse()*tan1*bump;
	  tan1 = qn.getVector()(1,2,3);
    ShColor3f diffuse = cond12*diffuse1 + (cond12 == 0.0)*diffuse2;
    ShVector3f tan2;
#else
    ShColor3f diffuse = diffuse1;
    ShAttrib2f tc = texcoord;
    
    ShVector3f tan2 = cross(normal, tan1);
    ShMatrix4x4f rot;
    
    rot[0](1) = tan2(0);
    rot[1](1) = tan2(1);
    rot[2](1) = tan2(2);
    rot[0](2) = normal(0);
    rot[1](2) = normal(1);
    rot[2](2) = normal(2);
    rot[0](0) = tan1(0);
    rot[1](0) = tan1(1);
    rot[2](0) = tan1(2);
#if (USE_QUAT)
    ShQuaternionf frame(quaternionMap(tc));
    ShVector3f zmap = ShVector3f(0,0,1);
    ShVector3f xmap = ShVector3f(1,0,0);
    ShMatrix4x4f mat = frame.getMatrix();
    zmap = mat | zmap;
    xmap = mat | xmap;
#else
    ShVector3f zmap = normalMap(tc);
    ShVector3f xmap = tangentMap(tc);
    xmap(2) = 0;
    xmap = normalize(cross(cross(xmap, zmap), zmap));
#endif
    ShVector3f ymap = cross(zmap, xmap);

    normal = (mapNormal > 0.5)*(rot | zmap) + (mapNormal <= 0.5)*normal;
    tan1 = (mapTangent > 0.5)*(rot | xmap) + (mapTangent <= 0.5)*tan1;
#endif
    tan2 = cross(normal, tan1);

    color = ashikhmin(nu, nv, normalize(normal), normalize(halfvec), 
        normalize(lightvec), normalize(viewvec), tan1, tan2, specular, diffuse)
      + ambient;
  } SH_END_PROGRAM;
    
  return true;
}


FrameMapping ash;
}

