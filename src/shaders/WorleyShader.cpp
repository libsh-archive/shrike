#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class WorleyShader : public Shader {
public:
  WorleyShader(std::string name, bool tex)
    : Shader(name + (tex ? " (Texture Hash)" : " (Procedural Hash)")), useTexture(tex) {}
  virtual ~WorleyShader() {}

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  // sets default vertex shader, phong fsh 
  bool init(); 

  // sets colours, specular, worley function specific to each subclass and uses algebra to 
  // add on to the default phong shader
  virtual void initfsh() = 0;

  ShProgram vsh, fsh; 

  // uniforms used by subclasses
  ShAttrib1f param, freq, exponent;
  /*static*/ ShAttrib4f coeff;
  ShColor3f color1, color2;

  //ShUtil::ShWorleyMetric metric;
  bool useTexture;
};
//ShAttrib4f WorleyShader::coeff;

bool WorleyShader::init() {
  std::cerr << "Initializing " << name() << std::endl;
  param.name("param"); 
  param = .5;         
  param.range(0.0f, 1.0f);

  freq.name("Worley frequency");
  freq = 16.0;
  freq.range(0.1f, 128.0f);

  coeff.name("Worley coefficient");
  coeff = ShConstAttrib4f(1.0, 0.0, 0.0, 0.0);
  coeff.range(-3.0f, 3.0f);

  exponent.name("specular exponent");
  exponent = 35.0;    
  exponent.range(10.0f, 500.0f);


  color1.name("color1");
  color1.range(-2.0f, 2.0f);
  color2.name("color2");
  color2.range(-2.0f, 2.0f);

  vsh = ShKernelLib::shVsh(Globals::mv, Globals::mvp, 1) << shExtract("lightPos") << Globals::lightPos; 

  ShConstColor3f lightColor(1.0f, 1.0f, 1.0f);
  fsh = ShKernelSurface::phong<ShColor3f>() << shExtract("specExp") << exponent;
  fsh = fsh << shExtract("irrad") << lightColor;

  initfsh();
  return true;
}
class GradientWorley: public WorleyShader {
  public:
    GradientWorley(bool useTexture): WorleyShader("Worley: Gradients", useTexture) {}

    void initfsh() {
      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqGradientPropFactory<2, float> propFactory; 

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory) << coeff << coeff << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(1.0f);
      bumpScale.range(-10.0f, 10.0f);

      color1 = ShColor3f(1.0, 0.0, 0.0);
      color2 =  ShColor3f(0.0, 1.0, 0.0);
      ShProgram color = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputAttrib3f SH_DECL(result);
        ShInputPosition4f SH_DECL(posh);
        ShOutputColor3f SH_NAMEDECL(resultColor, "result");
        resultColor = bumpScale * (color1 * result(1) + color2 * result(2)); 
      } SH_END;

      fsh = color << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};


class OrganicWorley: public WorleyShader {
  public:
    OrganicWorley(bool useTexture): WorleyShader("Worley: Organic", useTexture) {}

    void initfsh() {
      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqGradientPropFactory<2, float> propFactory; 

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory) << coeff << coeff << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(3.0f);
      bumpScale.range(-10.0f, 10.0f);

      color1 = ShColor3f(0.4, 0.0, 0.0);
      color2 =  ShColor3f(0.2, 0.6, 0.2);

      ShConstAttrib1f ZERO(0.0f);
      ShConstAttrib1f ONE(1.0f);
      ShProgram colorAndBump = SH_BEGIN_PROGRAM() {
        ShInputAttrib3f SH_DECL(result);
        ShInputVector3f SH_DECL(tangent) = normalize(tangent);
        ShInputVector3f SH_DECL(tangent2) = normalize(tangent2);
        ShInputVector3f SH_DECL(lightVec) = normalize(lightVec);

        ShInOutNormal3f SH_DECL(normal) = normalize(normal); 
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShOutputVector3f SH_DECL(halfVec);

        kd = ks = lerp(clamp(ZERO, ONE, result(0)), color1, color2);
        normal += bumpScale * (result(1) * tangent + result(2) * tangent2);
        normal = normalize(normal);
        halfVec = 0.5f * (normal + lightVec); 
      } SH_END;

      fsh = namedConnect((colorAndBump << worleysh), fsh);
      vsh = namedAlign(vsh, fsh);
    }
};

class PolkaDotWorley: public WorleyShader {
  public:
    PolkaDotWorley(bool useTexture): WorleyShader("Worley: Polka Dot", useTexture) {}

    void initfsh() {
      ShProgram worleysh = shWorley<4, 2, float>(useTexture) << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      // make polkadots by clamping the scalar result from worley
      ShProgram polkash = SH_BEGIN_PROGRAM() {
        ShInOutAttrib1f SH_DECL(scalar) = clamp(0.0f, 1.0f, (scalar + param - 0.75f) * 30.0f); 
      } SH_END;
      worleysh = polkash << worleysh;

      color1 = ShColor3f(0.27, 0.35, 0.45);
      color2 =  ShColor3f(1.0, 0.7, 0.0);
      ShColor3f specularColor(0.5, 0.5, 0.5);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

class LavaWorley: public WorleyShader {
  public:
    LavaWorley(bool useTexture): WorleyShader("Worley: Lava", useTexture) {}

    void initfsh() {
      coeff = ShConstAttrib4f(-1, 1.2, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstAttrib4f(0, 1, 1, 0);
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;

      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqPropFactory<2, float> distSqPropFactory;
      Dist_1PropFactory<2, float> dist_1PropFactory;

      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory) << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShProgram worleysh2 = shWorley<4>(&genFactory, &distSqPropFactory) << coeff2;
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq2));

      worleysh = sub<ShAttrib1f>() << namedCombine(worleysh, worleysh2);

      color1 = ShColor3f(0.0, 0.0, 1.0);
      color2 =  ShColor3f(0.5, 0.0, 0.0);
      ShColor3f specularColor(0.5, 0.5, 0.5);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

class GiraffeWorley: public WorleyShader {
  public:
    GiraffeWorley(bool useTexture): WorleyShader("Worley: Giraffe", useTexture) {}

    void initfsh() {
      param = ShConstAttrib1f(0.75);
      coeff = ShConstAttrib4f(-1, 1, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstAttrib4f(0, -1, 1, 0);

      DefaultGenFactory<2, float> genFactory(useTexture);
      Dist_1PropFactory<2, float> dist_1PropFactory;
      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory) << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShProgram worleysh2 = shWorley<4>(&genFactory, &dist_1PropFactory) << coeff2;
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      // make patches out of the two
      ShProgram patcher = SH_BEGIN_PROGRAM() {
        ShInputAttrib1f SH_DECL(worleyScalar1);
        ShInputAttrib1f SH_DECL(worleyScalar2);
        ShOutputAttrib1f SH_DECL(result);
        worleyScalar1 = (1.0f - worleyScalar1 - param) * 30.0f;
        worleyScalar2 = (1.0f - worleyScalar2 - param) * 10.0f;
        result = clamp(0.0f, 1.0f, worleyScalar1 + worleyScalar2);
      } SH_END;

      worleysh = patcher << namedCombine(worleysh, worleysh2);

      color1 =  ShColor3f(1.0, 0.95, 0.8);
      color2 = ShColor3f(0.45, 0.3, 0.0);
      ShColor3f specularColor(0.2, 0.2, 0.2);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

class CircuitWorley: public WorleyShader {
  public:
    CircuitWorley(bool useTexture): WorleyShader("Worley: Circuit", useTexture) {}

    void initfsh() {
      coeff = ShConstAttrib4f(0, 0, 0, 1);
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;

      DefaultGenFactory<2, float> genFactory(useTexture);
      Dist_1PropFactory<2, float> dist_1PropFactory;
      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory) << coeff;
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShProgram worleysh2 = shWorley<4>(&genFactory, &dist_1PropFactory) << coeff;
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq2));

      color1 = ShColor3f(0.0, 0.2, 0.8);
      color2 =  ShColor3f(0.6, 0.6, 0.7);

      // make patches out of the two
      ShProgram colorizer = SH_BEGIN_PROGRAM() {
        ShInputAttrib1f SH_DECL(worleyScalar1);
        ShInputAttrib1f SH_DECL(worleyScalar2);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);

        worleyScalar1 = clamp(0.0f, 1.0f, (1.0f - worleyScalar1 + param) * 30.0f);
        worleyScalar2 = 1.0f - clamp(0.0f, 1.0f, (1.0f - worleyScalar2 + param) * 30.0f);
        kd = worleyScalar1 * color1 + worleyScalar2 * color2; 
        ks = kd; 
      } SH_END;

      worleysh = namedCombine(worleysh, worleysh2);
      worleysh = colorizer << worleysh;
      fsh = fsh << worleysh; 
      vsh = namedAlign(vsh, fsh);
    }
};

class CrackedWorley: public WorleyShader {
  public:
    CrackedWorley(bool useTexture): WorleyShader("Worley: Cracked", useTexture) {}

    void initfsh() {
      ShAttrib1f SH_DECL(time); 
      time.range(0.0f, 9.0f);

      color1 = ShColor3f(3.0, 0.75, 0.0);
      color2 =  ShColor3f(0.0f, 0.0f, 0.0f);

      ShColor3f specularColor(0.5, 0.5, 0.5);

      coeff = ShConstAttrib4f(2.5, -0.5f, -0.1f, 0);
      freq = ShConstAttrib1f(16.0f);

      LerpGenFactory<2, float> genFactory(time, useTexture);
      DistSqPropFactory<2, float> propFactory;

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory) << coeff; // pass in coefficient
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShProgram clamper = SH_BEGIN_PROGRAM() {
        ShInOutAttrib1f SH_DECL(scalar) = clamp(0.0f, 1.0f, scalar);
      } SH_END;
      worleysh = clamper << worleysh;

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 
      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

class StoneWorley: public WorleyShader {
  public:
    StoneWorley(bool useTexture): WorleyShader("Worley: Stone Tile", useTexture) {}

    void initfsh() {
      color1 = ShColor3f(1.5, 0.75, 0.0);

      ShColor3f specularColor(0.25f, 0.25f, 0.25f);
      ShAttrib1f SH_DECL(threshold) = ShConstAttrib1f(0.1f);
      threshold.range(-1.0f, 1.0f);

      coeff = ShConstAttrib4f(-1.0f, 1.0f, 0.0f, 0.0f);
      ShAttrib4f noiseCoeff = ShConstAttrib4f(1.0f, 0.0f, 0.0f, 0.0f);

      freq = ShConstAttrib1f(16.0f);

      DefaultGenFactory<2, float> genFactory(useTexture);
      //NullGenFactory<2, float> genFactory;
      DistSqPropFactory<2, float> distPropFactory;
      CellnoisePropFactory<1, 2, float> noisePropFactory(useTexture);
      PropertyFactory<2, 2, float> *propFactory = combine(&distPropFactory, &noisePropFactory);

      ShProgram worleysh = shWorley<4>(&genFactory, propFactory) << coeff << noiseCoeff; // pass in coefficients 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShColor3f SH_DECL(noiseColor) = ShConstAttrib3f(0.1f, 0.2f, 0.2f);
      noiseColor.range(0.0f, 1.0f);

      ShAttrib1f SH_DECL(noiseFreq) = ShConstAttrib1f(64.0f);
      noiseFreq.range(0.0f, 768.0f);

      ShAttrib3f SH_DECL(turbulenceAmps) = ShConstAttrib3f(1.0f, 0.5f, 0.25f);
      turbulenceAmps.range(0.0f, 1.0f);

      ShProgram color = SH_BEGIN_PROGRAM() {
        ShInputTexCoord2f SH_DECL(texcoord);
        ShInputAttrib2f SH_DECL(result);
        ShInputVector3f SH_DECL(lightVec);

        ShInOutNormal3f SH_DECL(normal);
        ShOutputVector3f SH_DECL(halfVec);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);

        ShAttrib1f inGrout = result(0) < threshold;
        kd = noiseColor * sturbulence<1>(turbulenceAmps, texcoord * noiseFreq, useTexture);
        kd = lerp(inGrout, ShConstColor3f(1.0f, 1.0f, 1.0f), result(1,1,1) * color1 + kd);
        ks = specularColor; 
        //halfVec = 0.5*(normal + normalize(lightVec));
        //kd = sturbulence<3>(turbulenceAmps, texcoord * noiseFreq, useTexture);
      } SH_END;

      fsh = namedConnect(namedConnect(worleysh, color), fsh);
      vsh = namedAlign(vsh, fsh);
      
      delete propFactory;
    }
};

class TurbulentWorley: public WorleyShader {
  public:
    TurbulentWorley(bool useTexture): WorleyShader("Worley: Turbulence", useTexture) {}

    void initfsh() {
      ShAttrib4f SH_NAMEDECL(amps, "Octave_Amplitudes") = ShConstAttrib4f(1.0f, 0.5f, 0.25f, 0.125f);
      amps.range(0.0f, 1.0f);
      const int N = 4; // number of octaves

      color1 = ShColor3f(1.0f, 1.0f, 1.0f);
      color2 = ShColor3f(0.0f, 0.0f, 0.0f); 
      ShColor3f specularColor(0.5, 0.5, 0.5);

      coeff = ShConstAttrib4f(0.0f, 0.5f, 0.5f, 0.0f);
      freq = ShConstAttrib1f(16.0f);

      ShProgram worleysh[N];
      for(int i = N - 1; i >= 0; --i) {
        DefaultGenFactory<2, float> genFactory(useTexture);
        Dist_1PropFactory<2, float> distFactory;
        worleysh[i] = shWorley<4>(&genFactory, &distFactory) << coeff; 
        ShProgram multiplier = SH_BEGIN_PROGRAM() {
          ShInOutTexCoord2f SH_DECL(texcoord);
          texcoord *= freq * (float)(1 << i);
        } SH_END;
        worleysh[i] = worleysh[i] << multiplier; 
      }
      ShProgram comboWorley = worleysh[0];
      for(int i = 1; i < N; ++i) {
        comboWorley = namedCombine(comboWorley, worleysh[i]);
      }

      ShProgram colorizer = SH_BEGIN_PROGRAM() {
        ShInputAttrib1f SH_DECL(result[N]);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShAttrib1f weightedResult(0.0f); 
        for(int i = 0; i < N; ++i) weightedResult = mad(result[i], amps(i), weightedResult);
        kd = lerp(weightedResult, color1, color2);
        ks = kd;
      } SH_END;

      fsh = fsh << colorizer << comboWorley; 
      vsh = namedAlign(vsh, fsh);
    }
};

class MosaicWorley: public WorleyShader {
  public:
    MosaicWorley(bool useTexture): WorleyShader("Worley: Mosaic", useTexture) {}

    void initfsh() {
      ShColor3f specularColor(0.5, 0.5, 0.5);
      double myfreq = 64.0;

      freq = ShConstAttrib1f(myfreq);

      ShImage image;
      image.loadPng(SHMEDIA_DIR "/textures/kd.png");
      ShTexture2D<ShColor3f> mosaicTex(image.width(), image.height());
      mosaicTex.name("Mosaic Texture");
      mosaicTex.memory(image.memory());

      std::string imageNames[6] = {"left", "right", "top", "bottom", "back", "front"};
      ShImage test_image;
      test_image.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[0] + ".png");

      ShTextureCube<ShColor4f> cubemap(test_image.width(), test_image.height());
      {
        for (int i = 0; i < 6; i++) {
          ShImage image2;
          image2.loadPng(std::string(SHMEDIA_DIR "/envmaps/aniroom/") + imageNames[i] + ".png");
          cubemap.memory(image2.memory(), static_cast<ShCubeDirection>(i));
        }
      }

      ShAttrib1f SH_DECL(texScale) = ShConstAttrib1f(image.width()/myfreq);
      texScale.range(0.0f, 128.0f);

      DefaultGenFactory<2, float> genFactory(useTexture);
      //NullGenFactory<2, float> genFactory;
      DistSqPropFactory<2, float> distFactory;
      Tex2DPropFactory<ShColor3f, float> tex2dFactory(mosaicTex, texScale);

      coeff = ShConstAttrib4f(-1.0f, 1.0f, 0.0f, 0.0f);

      ShAttrib4f SH_DECL(colorCoeff) = ShConstAttrib4f(3.0f, 0.0f, 0.0f, 0.0f);
      colorCoeff.range(-4.0f, 4.0f);

      ShProgram worleysh = shWorley<4>(&genFactory, combine(&distFactory, &tex2dFactory)) 
        << coeff << colorCoeff << colorCoeff << colorCoeff; // pass in coefficient
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));


      ShAttrib1f theta = ShAttrib1f(1.3f);
      theta.name("relative indices of refraction");
      theta.range(0.0f,2.0f);

      ShAttrib1f SH_NAMEDECL(threshold, "Lattice threshold") = ShAttrib1f(0.1f);
      threshold.range(-1.0f, 1.0f);

      // TODO actually get the glass shader instead of rewriting it here
      ShProgram vshAddon = SH_BEGIN_PROGRAM("gpu:vertex") {
        ShInOutNormal3f SH_DECL(normal);
        ShInOutVector3f SH_DECL(viewVec);
        ShOutputVector3f SH_DECL(reflv);
        ShOutputVector3f SH_DECL(refrv);
        ShOutputAttrib1f SH_DECL(fres);
        
        reflv = reflect(viewVec,normal); // Compute reflection vector
        refrv = refract(viewVec,normal,theta); // Compute refraction vector
        fres = fresnel(viewVec,normal,theta); // Compute fresnel term

        // actually do reflection and refraction lookup in model space
        reflv = Globals::mv_inverse | reflv;
        refrv = Globals::mv_inverse | refrv;
      } SH_END;
      vsh = namedConnect(vsh, vshAddon, true);

      ShProgram perturber = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputAttrib4f SH_DECL(result); // worley result - scalar and RGB color

        ShOutputColor3f SH_DECL(color);
        ShInOutAttrib1f SH_DECL(fres);
        // stain colours inside mosaic tiles and at tile edges,
        // make it opaque and black
        ShAttrib1f inEdge = result(0) < threshold; 
        
        color = lerp(inEdge, ShConstColor3f(0.0f, 0.0f, 0.0f), result(1,2,3)); 
        fres = lerp(inEdge, ShConstAttrib1f(1.0f), fres); 
      } SH_END;
      
      fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputPosition4f SH_DECL(posh);
        ShInputColor3f SH_DECL(color); // stain color on glass
        ShInputNormal3f SH_DECL(normal);  // normal
        ShInputVector3f SH_DECL(reflv); // Compute reflection vector
        ShInputVector3f SH_DECL(refrv); // Compute refraction vector
        ShInputAttrib1f SH_DECL(fres); // Compute fresnel term

        ShOutputColor3f result;
        
        result = color * lerp(fres, cubemap(reflv)(0,1,2), cubemap(refrv)(0,1,2)); 
      } SH_END;

      fsh = namedConnect(perturber << worleysh, fsh); 
      vsh = namedAlign(vsh, fsh);
    }
}; 

class Worley3D: public WorleyShader {
  public:
    Worley3D(bool useTexture): WorleyShader("Worley: 3D", useTexture) {}

    void initfsh() {
      ShProgram worleysh = shWorley<1, 3, float>(useTexture) << ShConstAttrib1f(1.0f);
      worleysh = worleysh << (mul<ShTexCoord3f>("texcoord", "freq", "posv") << fillcast<3>(freq));

      // make polkadots by clamping the scalar result from worley
      color1 = ShColor3f(0.27, 0.35, 0.45);
      color2 =  ShColor3f(1.0, 0.7, 0.0);
      ShColor3f specularColor(0.5, 0.5, 0.5);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

class Worley2D: public WorleyShader {
  public:
    Worley2D(bool useTexture): WorleyShader("Worley: 2D", useTexture) {}

    void initfsh() {
      ShProgram worleysh = shWorley<4, 2, float>(useTexture) << coeff; 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      // make polkadots by clamping the scalar result from worley
      color1 =  ShColor3f(1.0, 1.0f, 1.0f); 
      color2 = ShColor3f(0.0f, 0.0f, 0.0f);
      ShColor3f specularColor(0.0, 0.0, 0.0);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};

CrackedWorley cracked(true);
CrackedWorley crackedProcedural(false);

//GradientWorley gradworley(true);
StoneWorley stone(true);
OrganicWorley organic(true);
TurbulentWorley turb(true);
MosaicWorley mosaic(true);
//Worley3D worley3d(true);
Worley2D worley2d(true);

PolkaDotWorley polka(false);
LavaWorley lava(false);
GiraffeWorley giraffe(false);
CircuitWorley circuit(false);
