#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

enum WorleyType {

};

class WorleyShader : public Shader {
public:
  WorleyShader(std::string name, bool tex)
    : Shader(name + (tex ? "(Texture Hash)" : "(Procedural Hash)")), useTexture(tex) {}
  virtual ~WorleyShader() {}

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  // sets default vertex shader, phong fsh 
  bool init(); 

  // returns a worley program that uses the given uniforms for coefficients 
  // and frequency, and the specified metric
  ShProgram makeWorleySh(ShWorleyMetric m, const ShAttrib4f &coeff, const ShAttrib1f &freq, bool keepGradients) const;

  // sets colours, specular, worley function specific to each subclass and uses algebra to 
  // add on to the default phong shader
  virtual void initfsh() = 0;

  ShProgram vsh, fsh; 

  // uniforms used by subclasses
  ShAttrib1f param, freq, exponent;
  ShAttrib4f coeff;
  ShColor3f color1, color2;

  ShUtil::ShWorleyMetric metric;
  bool useTexture;
};

bool WorleyShader::init() {
  std::cerr << "Initializing " << name() << std::endl;
  param.name("param"); 
  param = .5;         
  param.range(0.0f, 1.0f);

  freq.name("Worley frequency");
  freq = 8.0;
  freq.range(0.1f, 128.0f);

  coeff.name("Worley coefficient");
  coeff = ShConstant4f(1.0, 0.0, 0.0, 0.0);
  coeff.range(-3.0f, 3.0f);

  exponent.name("specular exponent");
  exponent = 35.0;    
  exponent.range(10.0f, 500.0f);


  color1.name("color1");
  color2.name("color2");

  vsh = ShKernelLib::shVshTangentSpace(Globals::mv, Globals::mvp, false) << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("texcoord", "normal", "halfVec", "lightVec", "posh") << vsh;
  fsh = ShKernelLib::shPhong<ShColor3f>() << shExtract("specExp") << exponent;

  initfsh();
  return true;
}

ShProgram WorleyShader::makeWorleySh(ShWorleyMetric m, const ShAttrib4f &c, const ShAttrib1f &f, bool keepGradients) const {
    ShProgram result = worleyProgram<4, float>(m, useTexture) << c; // pass in coefficient
    result = result << ( mul<ShTexCoord2f>("texcoord") << fillcast<2>(f)); // multiply by worley frequency
    if( !keepGradients ) result = shDrop("gradient") << result; 
    return result;
}

class OrganicWorley: public WorleyShader {
  public:
    OrganicWorley(bool useTexture): WorleyShader("Organic Worley", useTexture) {}

    void initfsh() {
      ShProgram worleysh = makeWorleySh(L2_SQ, coeff, freq, true); 

      color1 = ShColor3f(0.7, 0.2, 0.3);
      color2 =  ShColor3f(0.4, 0.0, 0.0);

      ShProgram colorAndBump = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorAndBump = (colorAndBump & (keep<ShColor3f>("ks") << fillcast<ShAttrib1f, ShColor3f>())) << shDup(); 
      colorAndBump = colorAndBump & ShKernelLib::bump(); 

      fsh = fsh << colorAndBump << worleysh;
    }
};

class PolkaDotWorley: public WorleyShader {
  public:
    PolkaDotWorley(bool useTexture): WorleyShader("Polka Dot Worley", useTexture) {}

    void initfsh() {
      ShProgram worleysh = makeWorleySh(L2_SQ, coeff, freq, false); 

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
    }
};

class LavaWorley: public WorleyShader {
  public:
    LavaWorley(bool useTexture): WorleyShader("Lava Worley", useTexture) {}

    void initfsh() {
      coeff = ShConstant4f(-1, 1.2, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstant4f(0, 1, 1, 0);
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;
      ShProgram worleysh = makeWorleySh(L1, coeff, freq, false); 
      ShProgram worleysh2 = makeWorleySh(L2_SQ, coeff2, freq2, false); 

      worleysh = sub<ShAttrib1f>() << namedCombine(worleysh, worleysh2);

      color1 = ShColor3f(0.0, 0.0, 1.0);
      color2 =  ShColor3f(0.5, 0.0, 0.0);
      ShColor3f specularColor(0.5, 0.5, 0.5);

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 

      fsh = fsh << colorsh << worleysh;
    }
};

class GiraffeWorley: public WorleyShader {
  public:
    GiraffeWorley(bool useTexture): WorleyShader("Giraffe Worley", useTexture) {}

    void initfsh() {
      param = ShConstant1f(0.75);
      coeff = ShConstant4f(-1, 1, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstant4f(0, -1, 1, 0);
      ShProgram worleysh = makeWorleySh(L1, coeff, freq, false); 
      ShProgram worleysh2 = makeWorleySh(L1, coeff2, freq, false); 

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
    }
};

class CircuitWorley: public WorleyShader {
  public:
    CircuitWorley(bool useTexture): WorleyShader("Circuit Worley", useTexture) {}

    void initfsh() {
      coeff = ShConstant4f(0, 0, 0, 1);
      ShProgram worleysh = makeWorleySh(L1, coeff, freq, false); 
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;
      ShProgram worleysh2 = makeWorleySh(L1, coeff, freq2, false); 

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
      worleysh = namedConnect(worleysh, colorizer);
      fsh = fsh << worleysh; 
    }
};

class CrackedWorley: public WorleyShader {
  public:
    CrackedWorley(bool useTexture): WorleyShader("Cracked Worley", useTexture) {}

    void initfsh() {
      ShAttrib1f SH_DECL(time); 
      time.range(0.0f, 6.0f);

      color1 = ShColor3f(3.0, 0.75, 0.0);
      color2 =  ShColor3f(0.0f, 0.0f, 0.0f);

      ShColor3f specularColor(0.5, 0.5, 0.5);

      coeff = ShConstant4f(2.5, -1.25, 0, 0);

      ShWorleyLerpingPointGen<float> generator(time);
      ShProgram worleysh = worleyProgram<4, float>(L2_SQ, useTexture, generator) << coeff; // pass in coefficient
      worleysh = worleysh << ( mul<ShTexCoord2f>("texcoord") << fillcast<2>(freq)); // multiply by worley frequency
      worleysh = shDrop("gradient") << worleysh; 

      ShProgram clamper = SH_BEGIN_PROGRAM() {
        ShInOutAttrib1f SH_DECL(scalar) = clamp(0.0f, 1.0f, scalar);
      } SH_END;
      worleysh = clamper << worleysh;

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 
      fsh = fsh << colorsh << worleysh;
    }
};

CrackedWorley cracked(true);
OrganicWorley organic(false);
PolkaDotWorley polka(false);
LavaWorley lava(false);
GiraffeWorley giraffe(false);
CircuitWorley circuit(false);
