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
    : Shader(std::string("Worley: ") + (tex ? " Texture Hash: " : " Procedural: ") + name), useTexture(tex) {}
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
    GradientWorley(bool useTexture): WorleyShader("Gradients", useTexture) {}

    void initfsh() {
      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqGradientPropFactory<2, float> propFactory; 

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory); 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(1.0f);
      bumpScale.range(-10.0f, 10.0f);

      color1 = ShColor3f(1.0, 0.0, 0.0);
      color2 =  ShColor3f(0.0, 1.0, 0.0);
      ShProgram color = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputAttrib4f result[3]; // result[0] holds distances, [1], [2] hold the gradient

        ShInputPosition4f SH_DECL(posh);
        ShOutputColor3f SH_NAMEDECL(resultColor, "result");
        
        ShAttrib2f gradient;
        gradient(0) = dot(result[1], coeff);
        gradient(1) = dot(result[2], coeff);
        resultColor = bumpScale * (color1 * gradient(0) + color2 * gradient(1)); 
      } SH_END;

      fsh = color << worleysh;
      vsh = namedAlign(vsh, fsh);
    }
};


class OrganicWorley: public WorleyShader {
  public:
    OrganicWorley(bool useTexture): WorleyShader("Organic", useTexture) {}

    void initfsh() {
      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqGradientPropFactory<2, float> propFactory; 

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory); 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(3.0f);
      bumpScale.range(-10.0f, 10.0f);

      color1 = ShColor3f(0.4, 0.0, 0.0);
      color2 =  ShColor3f(0.2, 0.6, 0.2);

      ShConstAttrib1f ZERO(0.0f);
      ShConstAttrib1f ONE(1.0f);

      // TODO when bump mapper kernel properly supports transformation btw
      // coordinate spaces and handles all used auxillary vectors (hv, etc.)
      // remove most of this code.
      ShProgram colorAndBump = SH_BEGIN_PROGRAM() {
        ShInputAttrib4f result[3]; // 0 holds distances, 1-2 hold gradient
        ShInputVector3f SH_DECL(tangent) = normalize(tangent);
        ShInputVector3f SH_DECL(tangent2) = normalize(tangent2);
        ShInputVector3f SH_DECL(lightVec) = normalize(lightVec);

        ShInOutNormal3f SH_DECL(normal) = normalize(normal); 
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShOutputVector3f SH_DECL(halfVec);

        ShAttrib1f dist = dot(coeff, result[0]);
        ShAttrib2f grad;
        grad(0) = dot(coeff, result[1]);
        grad(1) = dot(coeff, result[2]);

        kd = ks = lerp(clamp(dist, ZERO, ONE), color1, color2);
        normal += bumpScale * (grad(0) * tangent + grad(1) * tangent2);
        normal = normalize(normal);
        halfVec = 0.5f * (normal + lightVec); 
      } SH_END;

      fsh = namedConnect((colorAndBump << worleysh), fsh);
      vsh = namedAlign(vsh, fsh);
    }
};

class BlueOrbWorley: public WorleyShader {
  public:
    BlueOrbWorley(bool useTexture): WorleyShader("BlueOrb", useTexture) {}

    void initfsh() {
      DefaultGenFactory<2, float> genFactory(useTexture);
      Dist_InfGradientPropFactory<2, float> propFactory; 
      Dist_1PropFactory<2, float> dist1Factory;

      coeff = ShConstAttrib4f(-1.0f, 1.0f, 0.0f, 0.0f);

      ShAttrib1f SH_DECL(frequency) = ShConstAttrib1f(16.0f);
      frequency.range(0.0f, 256.0f);

      ShProgram worleysh = shWorley<4>(&genFactory, &propFactory); 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib4f SH_DECL(innerCoeff) = coeff; 

      ShAttrib1f SH_DECL(innerFreq) = ShConstAttrib1f(32.0f);
      innerFreq.range(0.0f, 256.0f);

      ShProgram innersh = shWorley<4>(&genFactory, &dist1Factory);
      innersh = innersh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(innerFreq));

      worleysh = namedCombine(worleysh, innersh);

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(1.0f);
      bumpScale.range(-10.0f, 10.0f);

      ShColor3f SH_DECL(latticeColor) = ShColor3f(0.0f, 0.0f, 0.0f);
      ShColor3f SH_DECL(latticeSpecular) = ShColor3f(0.5f, 0.1f, 0.1f);

      color1 = ShColor3f(0.0f, 0.0f, 0.0f);
      color2 = ShColor3f(0.25f, 0.25f, 1.0f);
      ShColor3f SH_DECL(specular) = ShColor3f(0.3f, 0.3f, 0.3f);

      ShAttrib1f SH_NAMEDECL(threshold, "Less Than Threshold") = ShConstAttrib1f(0.2f);
      threshold.range(-1.0f, 1.0f);

      ShAttrib1f SH_NAMEDECL(threshold2, "Greater Than Threshold") = ShConstAttrib1f(1.0f);
      threshold2.range(-1.0f, 1.0f);

      ShConstAttrib1f ZERO(0.0f);
      ShConstAttrib1f ONE(1.0f);
      ShProgram colorAndBump = SH_BEGIN_PROGRAM() {
        ShInputAttrib4f result[3];
        ShInputAttrib4f innerResult;
        ShInputVector3f SH_DECL(tangent) = normalize(tangent);
        ShInputVector3f SH_DECL(tangent2) = normalize(tangent2);
        ShInputVector3f SH_DECL(lightVec) = normalize(lightVec);

        ShInOutNormal3f SH_DECL(normal) = normalize(normal); 
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShOutputVector3f SH_DECL(halfVec);


        // determine if we're in one of the bumps in the outer lattice work
        ShAttrib1f dist = coeff | result[0];
        ShAttrib1f inBump = (dist < threshold) + (dist > threshold2); 

        // figure out inner colour and select kd/ks based on whether we're in the bumps 
        ShColor3f innerColor = lerp(innerCoeff | innerResult, color1, color2);
        kd = lerp(inBump, latticeColor, innerColor); 
        ks = lerp(inBump, latticeSpecular, specular);

        // do the bump mapping (TODO use bump function/nibble)
        ShAttrib2f grad;
        grad(0) = dot(coeff, result[1]);
        grad(1) = dot(coeff, result[2]);

        normal += inBump * bumpScale * (grad(0) * tangent + grad(1) * tangent2);
        normal = normalize(normal);
        halfVec = 0.5f * (normal + lightVec); 
      } SH_END;

      fsh = namedConnect((colorAndBump << worleysh), fsh);
      vsh = namedAlign(vsh, fsh);
    }
};

class PolkaDotWorley: public WorleyShader {
  public:
    PolkaDotWorley(bool useTexture): WorleyShader("Polka Dot", useTexture) {}

    void initfsh() {
      ShProgram worleysh = shWorley<4, 2, float>(useTexture);
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh = (dot<ShAttrib4f>() << coeff) << worleysh;

      // make polkadots by clamping the scalar result from worley
      ShProgram polkash = SH_BEGIN_PROGRAM() {
        ShInOutAttrib1f SH_DECL(scalar) = clamp((scalar + param - 0.75f) * 30.0f, 0.0f, 1.0f); 
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
    LavaWorley(bool useTexture): WorleyShader("Lava", useTexture) {}

    void initfsh() {
      coeff = ShConstAttrib4f(-1, 1.2, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstAttrib4f(0, 1, 1, 0);
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;

      DefaultGenFactory<2, float> genFactory(useTexture);
      DistSqPropFactory<2, float> distSqPropFactory;
      Dist_1PropFactory<2, float> dist_1PropFactory;

      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory);
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh = (dot<ShAttrib4f>() << coeff) << worleysh;

      ShProgram worleysh2 = shWorley<4>(&genFactory, &distSqPropFactory);
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq2));
      worleysh2 = (dot<ShAttrib4f>() << coeff2) << worleysh2;

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
    GiraffeWorley(bool useTexture): WorleyShader("Giraffe", useTexture) {}

    void initfsh() {
      param = ShConstAttrib1f(0.75);
      coeff = ShConstAttrib4f(-1, 1, 0, 0);
      ShAttrib4f SH_NAMEDECL(coeff2, "Worley coefficient 2") = ShConstAttrib4f(0, -1, 1, 0);

      DefaultGenFactory<2, float> genFactory(useTexture);
      Dist_1PropFactory<2, float> dist_1PropFactory;
      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory);
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh = (dot<ShAttrib4f>() << coeff) << worleysh;

      ShProgram worleysh2 = shWorley<4>(&genFactory, &dist_1PropFactory);
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh2 = (dot<ShAttrib4f>() << coeff2) << worleysh2;

      // make patches out of the two
      ShProgram patcher = SH_BEGIN_PROGRAM() {
        ShInputAttrib1f SH_DECL(worleyScalar1);
        ShInputAttrib1f SH_DECL(worleyScalar2);
        ShOutputAttrib1f SH_DECL(result);
        worleyScalar1 = (1.0f - worleyScalar1 - param) * 30.0f;
        worleyScalar2 = (1.0f - worleyScalar2 - param) * 10.0f;
        result = clamp(worleyScalar1 + worleyScalar2, 0.0f, 1.0f);
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
    CircuitWorley(bool useTexture): WorleyShader("Circuit", useTexture) {}

    void initfsh() {
      coeff = ShConstAttrib4f(0, 0, 0, 1);
      ShAttrib1f SH_NAMEDECL(freq2, "Worley frequency 2") = freq * 2.131313f;

      DefaultGenFactory<2, float> genFactory(useTexture);
      Dist_1PropFactory<2, float> dist_1PropFactory;
      ShProgram worleysh = shWorley<4>(&genFactory, &dist_1PropFactory);
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh = (dot<ShAttrib4f>() << coeff) << worleysh;

      ShProgram worleysh2 = shWorley<4>(&genFactory, &dist_1PropFactory);
      worleysh2 = worleysh2 << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq2));
      worleysh2 = (dot<ShAttrib4f>() << coeff) << worleysh2;

      color1 = ShColor3f(0.0, 0.2, 0.8);
      color2 =  ShColor3f(0.6, 0.6, 0.7);

      // make patches out of the two
      ShProgram colorizer = SH_BEGIN_PROGRAM() {
        ShInputAttrib1f SH_DECL(worleyScalar1);
        ShInputAttrib1f SH_DECL(worleyScalar2);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);

        worleyScalar1 = clamp((1.0f - worleyScalar1 + param) * 30.0f, 0.0f, 1.0f);
        worleyScalar2 = 1.0f - clamp((1.0f - worleyScalar2 + param) * 30.0f, 0.0f, 1.0f);
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
    bool m_animate;
    ShAttrib1f m_old, m_enable, m_time, m_speed;

    CrackedWorley(bool useTexture, bool animate) 
      : WorleyShader(std::string("Cracked") + (animate ? " Animating" : ""), useTexture), 
        m_animate(animate) {
      m_old = m_enable = 1.0f;
      m_enable.name("Enable Animation");

      m_speed = 0.1f;
      m_speed.name("Animation Speed");
      m_speed.range(-2.0f, 2.0f);

      m_time = 0.0f;
    }

    void initfsh() {
      color1 = ShColor3f(3.0, 0.75, 0.0);
      color2 =  ShColor3f(0.0f, 0.0f, 0.0f);

      ShColor3f specularColor(0.5, 0.5, 0.5);

      coeff = ShConstAttrib4f(2.5, -0.5f, -0.1f, 0);
      freq = ShConstAttrib1f(16.0f);

      ShProgram worleysh;

      DistSqPropFactory<2, float> propFactory;

      if (m_animate) {
        LerpGenFactory<2, float> genFactory(m_time, useTexture);
        worleysh = shWorley<4>(&genFactory, &propFactory);
      } else {
        DefaultGenFactory<2, float> genFactory(useTexture);
        worleysh = shWorley<4>(&genFactory, &propFactory); 
      }

      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));
      worleysh = (dot<ShAttrib4f>() << coeff) << worleysh;

      ShProgram clamper = SH_BEGIN_PROGRAM() {
        ShInOutAttrib1f SH_DECL(scalar) = clamp(scalar, 0.0f, 1.0f);

        // TODO get rid of this hack... This makes m_speed and m_enable visible in the uniform panel
        scalar += 0.0f * (m_speed + m_enable);
      } SH_END;
      worleysh = clamper << worleysh;

      ShProgram colorsh = lerp<ShColor3f, ShAttrib1f>("kd") << color1 << color2;  // kd is a lerp based on the worley scalar
      colorsh = colorsh & ( keep<ShColor3f>("ks") << specularColor); 
      fsh = fsh << colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }

    void render() {
      if((m_enable != m_old).getValue(0) > 0.5f) {
        m_time += m_speed;
        m_old = m_enable;
      }
      Shader::render();
    }

};

class StoneWorley: public WorleyShader {
  public:
    StoneWorley(bool useTexture): WorleyShader("Stone Tile", useTexture) {}

    void initfsh() {
      color1 = ShColor3f(1.0, 0.5, 0.0);

      ShColor3f specularColor(0.1f, 0.1f, 0.1f);
      ShAttrib1f SH_NAMEDECL(groutThreshold, "Grout Threshold") = ShConstAttrib1f(0.05f);
      groutThreshold.range(-1.0f, 1.0f);

      ShAttrib1f SH_NAMEDECL(bevelThreshold, "Bump Threshold") = ShConstAttrib1f(0.2f);
      bevelThreshold.range(-1.0f, 1.0f);

      coeff = ShConstAttrib4f(-1.0f, 1.0f, 0.0f, 0.0f);
      ShAttrib4f noiseCoeff = ShConstAttrib4f(1.0f, 0.0f, 0.0f, 0.0f);

      freq = ShConstAttrib1f(16.0f);

      DefaultGenFactory<2, float> genFactory(useTexture);
      //NullGenFactory<2, float> genFactory;
      DistSqGradientPropFactory<2, float> distPropFactory;
      CellnoisePropFactory<1, 2, float> noisePropFactory(useTexture);
      PropertyFactory<4, 2, float> *propFactory = combine(&distPropFactory, &noisePropFactory);

      ShProgram worleysh = shWorley<4>(&genFactory, propFactory); // pass in coefficients 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      ShAttrib1f SH_DECL(noiseScale) = ShConstAttrib1f(0.1f);
      noiseScale.range(0.0f, 1.0f);

      ShAttrib1f SH_DECL(noiseFreq) = ShConstAttrib1f(128.0f);
      noiseFreq.range(0.0f, 768.0f);

      ShAttrib3f SH_DECL(noiseAmps) = ShConstAttrib3f(1.0f, 0.5f, 0.25f);
      noiseAmps.range(0.0f, 1.0f);

      ShAttrib1f SH_DECL(bumpScale) = ShConstAttrib1f(3.0f);
      bumpScale.range(-10.0f, 10.0f);


      ShProgram worleyWeight = SH_BEGIN_PROGRAM() {
        // result[0] holds the 4-nearest distances, 
        // result[1,2] holds the gradient for 4 nearest features 
        // result[3] holds noise property for 4 nearest features
        ShInputAttrib4f result[4]; 

        // change from squared euclidean to euclidean distances
        for(int i = 0; i < 4; ++i) result[0](i) = sqrt(result[0](i));

        // calculate weighted sums of distances, gradients and noise property
        ShOutputAttrib1f SH_DECL(worleyDist) = dot(coeff, result[0]); 
        ShOutputAttrib2f SH_DECL(worleyGrad);
// TODO clean up this bump hack -coeff gives the right bumps, but
// we should really be picking coeff correctly and fixing thresholds
// below instead
        worleyGrad(0) = dot(-coeff, result[1]); 
        worleyGrad(1) = dot(-coeff, result[2]);
        ShOutputAttrib1f SH_DECL(worleyNoise) = dot(noiseCoeff, result[3]);
      } SH_END;

      worleysh = worleyWeight << worleysh;

      ShProgram color = SH_BEGIN_PROGRAM() {
        ShInputTexCoord2f SH_DECL(texcoord);
        ShInputAttrib1f SH_DECL(worleyDist);
        ShInputAttrib2f SH_DECL(worleyGrad);
        ShInputAttrib1f SH_DECL(worleyNoise);

        ShInputVector3f SH_DECL(tangent) = normalize(tangent);
        ShInputVector3f SH_DECL(tangent2) = normalize(tangent2);
        ShInputVector3f SH_DECL(lightVec) = normalize(lightVec);

        ShInOutNormal3f SH_DECL(normal);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShOutputVector3f SH_DECL(halfVec);

        // decide whether we're in grout or in the bevel near the grout
        ShAttrib1f inGrout = worleyDist < groutThreshold;
        ShAttrib1f inBevel = (worleyDist < (groutThreshold + bevelThreshold)) - inGrout;

        // generate stone colors (one per tile) 
        kd = lerp(inGrout, ShConstColor3f(1.0f, 1.0f, 1.0f), worleyNoise * color1);
        ks = specularColor; 

        // apply bump mapping
        ShVector3f inBevelPerturb = (worleyGrad(0) * tangent + worleyGrad(1) * tangent2);
        ShVector3f insideTilePerturb = noiseScale * sperlin<3>(texcoord * noiseFreq, noiseAmps, useTexture);
        normal += bumpScale * (
            lerp(inBevel, inBevelPerturb, ShConstVector3f(0.0f, 0.0f, 0.0f)) +
            lerp(inGrout, ShConstVector3f(0.0f, 0.0f, 0.0f), insideTilePerturb)); 
        normal = normalize(normal);
        halfVec = 0.5f * (normal + lightVec); 
      } SH_END;

      fsh = namedConnect(namedConnect(worleysh, color), fsh);
      vsh = namedAlign(vsh, fsh);
      
      delete propFactory;
    }
};

class TurbulentWorley: public WorleyShader {
  public:
    TurbulentWorley(bool useTexture): WorleyShader("Turbulence", useTexture) {}

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
        worleysh[i] = shWorley<4>(&genFactory, &distFactory); 
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
        ShInputAttrib4f SH_DECL(result[N]);
        ShOutputColor3f SH_DECL(kd);
        ShOutputColor3f SH_DECL(ks);
        ShAttrib1f weightedResult(0.0f); 
        for(int i = 0; i < N; ++i) weightedResult = mad(dot(result[i], coeff), amps(i), weightedResult);
        kd = lerp(weightedResult, color1, color2);
        ks = kd;
      } SH_END;

      fsh = fsh << colorizer << comboWorley; 
      vsh = namedAlign(vsh, fsh);
    }
};

class MosaicWorley: public WorleyShader {
  public:
    MosaicWorley(bool useTexture): WorleyShader("Mosaic", useTexture) {}

    void initfsh() {
      ShColor3f specularColor(0.5, 0.5, 0.5);

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

      ShAttrib1f SH_DECL(texScale) = ShConstAttrib1f(32.0);
      texScale.range(0.0f, image.width() / 16.0f);

      DefaultGenFactory<2, float> genFactory(useTexture);
      //NullGenFactory<2, float> genFactory;
      DistSqPropFactory<2, float> distFactory;
      Tex2DPropFactory<ShColor3f, float> tex2dFactory(mosaicTex, texScale);

      coeff = ShConstAttrib4f(-1.0f, 1.0f, 0.0f, 0.0f);

      ShAttrib4f SH_DECL(colorCoeff) = ShConstAttrib4f(3.0f, 0.0f, 0.0f, 0.0f);
      colorCoeff.range(-4.0f, 4.0f);

      ShProgram worleysh = shWorley<4>(&genFactory, combine(&distFactory, &tex2dFactory));
      ShProgram texcoordScaler = SH_BEGIN_PROGRAM() {
        ShInOutTexCoord2f SH_DECL(texcoord) = texcoord * (image.width() / texScale);
      } SH_END;
      worleysh = worleysh << texcoordScaler; 


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

      ShAttrib1f SH_DECL(killThreshold) = -0.1f;
      killThreshold.range(-16.0f, 16.0f);

      ShProgram perturber = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputAttrib4f result[4]; // worley result - result[0] = distance and result[1-3] = RGB color

        ShOutputColor3f SH_DECL(color);
        ShInOutAttrib1f SH_DECL(fres);

        // take square roots to get real euclidean distance
        for(int i = 0; i < 4; ++i) result[0](i) = sqrt(result[0](i));
        
        // stain colours inside mosaic tiles and at tile edges,
        // make it opaque and black
        ShAttrib1f dist = dot(result[0], coeff);
        ShAttrib1f inEdge = dist < threshold; 
        
        for(int i = 0; i < 3; ++i) color(i) = dot(result[i+1], colorCoeff);
        kill((color|ShConstAttrib3f(1.0f, 1.0f, 1.0f)) < killThreshold);
        color = lerp(inEdge, ShConstColor3f(0.0f, 0.0f, 0.0f), color); 
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
    Worley3D(bool useTexture): WorleyShader("3D", useTexture) {}

    void initfsh() {
      ShProgram worleysh = shWorley<1, 3, float>(useTexture);  // only keep closest neighbour
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

// replace these with inline nibbles later on
class Worley2D: public WorleyShader {
  public:
    Worley2D(bool useTexture, ShConstAttrib4f c, PropertyFactory<1, 2, float> *distFactory, std::string name)
      : WorleyShader(std::string("2D: ") + name, useTexture),
        m_distFactory(distFactory),
        m_coeff(c) {
      m_time.name("time");
      m_time.range(0.0f, 2.0f);
    }

    ~Worley2D() {
      delete m_distFactory;
    }
    // Lerp factory that starts with a fixed grid at time = 0 (good to see how
    // grid jittering works)
    struct FixedLerpFactory: GridGenFactory<2, float> {
      FixedLerpFactory(const ShGeneric<1, float> &time, bool useTexture)
        : m_time(time), m_useTexture(useTexture){}

      private:
        const ShGeneric<1, float> &m_time;
        bool m_useTexture;

        void makePos(Generator<2, float> &g) const {
          ShAttrib1f lastTime = floor(m_time);
          ShAttrib1f timeOffset = frac(m_time);
          ShAttrib3f offsetCell;

          offsetCell = fillcast<3>(g.cell);
          offsetCell(2) = lastTime;
          ShAttrib2f p1 = cellnoise<2>(offsetCell, m_useTexture);
          p1 = lerp(lastTime < 0.5f, ShConstAttrib2f(0.0f, 0.0f), p1); 

          offsetCell(2) += 1;
          ShAttrib2f p2 = cellnoise<2>(offsetCell, m_useTexture);

          g.pos = g.cell + lerp(timeOffset, p2, p1);
        }
    };

    void initfsh() {
      coeff = m_coeff;
      FixedLerpFactory genFactory(m_time, useTexture);

      DistSqPropFactory<2, float> euclideanDistFactory; 
      PropertyFactory<2, 2, float> *propFactory = combine(m_distFactory, &euclideanDistFactory); 

      ShProgram worleysh = shWorley<4>(&genFactory, propFactory); 
      worleysh = worleysh << (mul<ShTexCoord2f>("texcoord", "freq", "texcoord") << fillcast<2>(freq));

      // make polkadots by clamping the scalar result from worley
      color1 =  ShColor3f(1.0, 1.0f, 1.0f); 
      color2 = ShColor3f(0.0f, 0.0f, 0.0f);

      ShAttrib1f SH_DECL(threshold) = 0.01f;
      threshold.range(-1.0f, 1.0f);

      ShColor3f SH_DECL(featureColor) = ShConstColor3f(1.0f, 0.0f, 1.0f);

      ShProgram colorsh = SH_BEGIN_PROGRAM("gpu:fragment") {
        ShInputAttrib4f dists; // 4-nearest worley distances
        ShInputAttrib4f euclideanDists; // 4-nearest euclidean distances
        ShInputPosition4f SH_DECL(posh);
        ShOutputColor3f SH_DECL(result);

        result = lerp(coeff | dists, color1, color2);
        result = cond(euclideanDists(0) < threshold, featureColor, result);
      } SH_END; 
      colorsh = colorsh; 

      fsh = colorsh << worleysh;
      vsh = namedAlign(vsh, fsh);
    }

    static ShAttrib1f m_time;
    PropertyFactory<1, 2, float> *m_distFactory;
    ShConstAttrib4f m_coeff;
};
ShAttrib1f Worley2D::m_time = 1.0f;

// Basic Examples:
Worley2D worley2dNothing(true, ShConstAttrib4f(0.0f, 0.0f, 0.0f, 0.0f), new DistSqPropFactory<2, float>(), "Points Only");
Worley2D worley2dF1(true, ShConstAttrib4f(1.0f, 0.0f, 0.0f, 0.0f), new DistSqPropFactory<2, float>(), "Euclidean Distance Squared: F1");
Worley2D worley2dF2(true, ShConstAttrib4f(0.0f, 1.0f, 0.0f, 0.0f), new DistSqPropFactory<2, float>(), "Euclidean Distance Squared: F2");
Worley2D worley2dF3(true, ShConstAttrib4f(0.0f, 0.0f, 1.0f, 0.0f), new DistSqPropFactory<2, float>(), "Euclidean Distance Squared: F3");
Worley2D worley2dF4(true, ShConstAttrib4f(0.0f, 0.0f, 0.0f, 1.0f), new DistSqPropFactory<2, float>(), "Euclidean Distance Squared: F4");

Worley2D worley2dL1F1(true, ShConstAttrib4f(1.0f, 0.0f, 0.0f, 0.0f), new Dist_1PropFactory<2, float>(), "Manhattan(L1) Distance: F1");
Worley2D worley2dL1F2(true, ShConstAttrib4f(0.0f, 1.0f, 0.0f, 0.0f), new Dist_1PropFactory<2, float>(), "Manhattan(L1) Distance: F2");
Worley2D worley2dL1F3(true, ShConstAttrib4f(0.0f, 0.0f, 1.0f, 0.0f), new Dist_1PropFactory<2, float>(), "Manhattan(L1) Distance: F3");
Worley2D worley2dL1F4(true, ShConstAttrib4f(0.0f, 0.0f, 0.0f, 1.0f), new Dist_1PropFactory<2, float>(), "Manhattan(L1) Distance: F4");

Worley2D worley2dInfF1(true, ShConstAttrib4f(1.0f, 0.0f, 0.0f, 0.0f), new Dist_InfPropFactory<2, float>(), "L Infinity Distance: F1");
Worley2D worley2dInfF2(true, ShConstAttrib4f(0.0f, 1.0f, 0.0f, 0.0f), new Dist_InfPropFactory<2, float>(), "L Infinity Distance: F2");
Worley2D worley2dInfF3(true, ShConstAttrib4f(0.0f, 0.0f, 1.0f, 0.0f), new Dist_InfPropFactory<2, float>(), "L Infinity Distance: F3");
Worley2D worley2dInfF4(true, ShConstAttrib4f(0.0f, 0.0f, 0.0f, 1.0f), new Dist_InfPropFactory<2, float>(), "L Infinity Distance: F4");

Worley3D worley3d(true);

// Thresholding Examples 
// (Euclidean and L1 metrics)
PolkaDotWorley polka(false);
GiraffeWorley giraffe(false);
CircuitWorley circuit(false);

// Fractal examples
LavaWorley lava(false);
TurbulentWorley turb(true);

// Bump Mapping examples
OrganicWorley organic(true);
BlueOrbWorley borg(true);
//GradientWorley gradworley(true);

// Animated Examples
CrackedWorley crackedAnim(true, true);
CrackedWorley crackedProceduralAnim(false, true);
//CrackedWorley cracked(true, false);
//CrackedWorley crackedProcedural(false, false);

// Cell-colouring examples (attaching random and texture-lookup
// cell colours to feature points)
StoneWorley stone(true);
MosaicWorley mosaic(true);
MosaicWorley mosaic2(false);


