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

enum NoiseType {

};

class NoiseShader : public Shader {
public:
  NoiseShader(std::string name, bool tex)
    : Shader(name + (tex ? " (Texture Hash)" : " (Procedural Hash)")), useTexture(tex) {}
  virtual ~NoiseShader() {}

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  // sets default vertex shader, phong fsh 
  bool init(); 

  // connects a banding function and noise function to the phong fragment shader 
  virtual void initfsh() = 0;

  ShProgram vsh, fsh; 

  // uniforms used by subclasses
  ShAttrib3f bandFreq; // frequency of teh bands
  ShAttrib3f noiseFreq; // frequency of the noise
  ShAttrib1f noiseScale; // scaling on the noise
  ShAttrib1f exponent; // specular specExp

  ShMatrix4x4f bandTrans; // band transformation matrix

  bool useTexture;
};

bool NoiseShader::init() {
  std::cerr << "Initializing " << name() << std::endl;
  bandFreq.name("Band Frequency");
  bandFreq = ShConstAttrib3f(4.0f, 4.0f, 4.0f);
  bandFreq.range(0.02f, 20.0f);

  noiseFreq.name("Noise Frequency");
  noiseFreq = ShConstAttrib3f(4.0f, 4.0f, 4.0f);
  noiseFreq.range(0.02f, 20.0f);

  noiseScale.name("Noise Scale");
  noiseScale = 0.1f;
  noiseScale.range(0.01f, 10.0f);

  exponent.name("Specular Exponent");
  exponent = 48.0f;
  exponent.range(1.0f, 256.0f);

  vsh = ShKernelLib::shVsh(Globals::mv, Globals::mvp) << shExtract("lightPos") << Globals::lightPos; 
  vsh = shSwizzle("normal", "halfVec", "lightVec", "posh") << vsh;
  vsh = namedCombine(transform<ShPoint3f>(bandTrans, "posm") << cast<ShPosition4f, ShPoint3f>("posm"), vsh);

  fsh = ShKernelSurface::phong<ShColor3f>(); 
  fsh = fsh << shExtract("irrad") << ShConstAttrib3f(1.0f, 1.0f, 1.0f);

  initfsh();
  return true;
}

class SimpleWoodNoise: public NoiseShader {
public:
  SimpleWoodNoise(bool useTexture): NoiseShader("Noise: Simple Wood", useTexture) {}

  void initfsh() {
    ShColor3f SH_NAMEDECL(diffuseIn, "kd in band") = ShConstAttrib3f(0.35f, 0.177f, 0.07f);
    ShColor3f SH_NAMEDECL(diffuseOut, "kd out of band") = ShConstAttrib3f(0.8f, 0.3f, 0.1f);
    ShColor3f SH_NAMEDECL(specularIn, "ks in band") = ShConstAttrib3f(0.3f, 0.3f, 0.3f);
    ShColor3f SH_NAMEDECL(specularOut, "ks out of band") = ShConstAttrib3f(1.0f, 1.0f, 1.0f);

    ShProgram bander = SH_BEGIN_PROGRAM() {
      ShInputPoint3f SH_DECL(posm);

      posm *= bandFreq;
      ShAttrib1f inband = sqrt(posm(0,1) | posm(0,1)); // concentric rings
      ShAttrib1f noise = noiseScale * sperlin<1>(posm * noiseFreq, useTexture);
      inband = frac(inband + noise); // add noise

      ShOutputColor3f SH_DECL(kd) = lerp(inband, diffuseIn, diffuseOut);
      ShOutputColor3f SH_DECL(ks) = lerp(inband, specularIn, specularOut);
      ShOutputAttrib1f SH_DECL(specExp) = exponent;
    } SH_END_PROGRAM;
    fsh = fsh << bander;
  }
};

class ComplexWoodNoise: public NoiseShader {
public:
  ComplexWoodNoise(bool useTexture): NoiseShader("Noise: Complex Wood", useTexture) {}

  void initfsh() {
    ShColor3f SH_NAMEDECL(diffuseIn, "kd in band") = ShConstAttrib3f(0.35f, 0.177f, 0.07f);
    ShColor3f SH_NAMEDECL(diffuseOut, "kd out of band") = ShConstAttrib3f(0.8f, 0.3f, 0.1f);
    ShColor3f SH_NAMEDECL(specularIn, "ks in band") = ShConstAttrib3f(0.3f, 0.3f, 0.3f);
    ShColor3f SH_NAMEDECL(specularOut, "ks out of band") = ShConstAttrib3f(1.0f, 1.0f, 1.0f);

    ShAttrib3f SH_NAMEDECL(turbAmp, "Noise Octave Amplitudes") = ShConstAttrib3f(0.5f, 0.25f, 0.125f);
    turbAmp.range(0.0f, 1.0f);

    ShAttrib1f SH_NAMEDECL(spaceNoiseFreq, "Band Spacing Noise Freq") = ShConstAttrib1f(4.0f);
    spaceNoiseFreq.range(0.02f, 20.0f);

    ShAttrib1f SH_NAMEDECL(spaceNoiseScale, "Band Spacing Noise Scale") = 0.15f;
    spaceNoiseScale.range(0.1f, 1.0f);

    ShProgram bander = SH_BEGIN_PROGRAM() {
      ShInputPoint3f SH_DECL(posm);

      posm *= bandFreq;
      ShAttrib1f inband = sqrt(posm(0,1) | posm(0,1)); // concentric rings
      ShAttrib1f spacenoise = spaceNoiseScale * sperlin<1>(inband * spaceNoiseFreq, useTexture);  
      ShAttrib1f noise = noiseScale * sturbulence<1>(posm * noiseFreq, turbAmp, useTexture);
      inband = frac(inband + noise + spacenoise); // add noise

      ShOutputColor3f SH_DECL(kd) = lerp(inband, diffuseIn, diffuseOut);
      ShOutputColor3f SH_DECL(ks) = lerp(inband, specularIn, specularOut);
      ShOutputAttrib1f SH_DECL(specExp) = exponent;
    } SH_END_PROGRAM;
    fsh = fsh << bander;
  }
};

class MarbleNoise: public NoiseShader {
public:
  MarbleNoise(bool useTexture): NoiseShader("Noise: Marble", useTexture) {}

  void initfsh() {
    noiseScale = 1.0f;
    ShColor3f SH_NAMEDECL(outColor, "Color out of band") = ShConstAttrib3f(1.0f, 0.9f, 1.0f);
    ShColor3f SH_NAMEDECL(color1, "Color in band1") = ShConstAttrib3f(0.2f, 0.1f, 0.0f);
    ShColor3f SH_NAMEDECL(color2, "Color in band2") = ShConstAttrib3f(0.3f, 0.4f, 0.9f);
    ShAttrib2f SH_NAMEDECL(bandrange, "Band Rnage") = ShConstAttrib2f(0.3f, 0.4f); 
    ShAttrib1f SH_NAMEDECL(width1, "Width of band1" ) = ShConstAttrib1f(0.3f);
    ShAttrib1f SH_NAMEDECL(width2, "Width of band2" ) = ShConstAttrib1f(0.2f);
    ShColor3f SH_NAMEDECL(specular, "specular") = ShConstAttrib3f(0.5f, 0.5f, 0.5f);
    ShAttrib2f SH_NAMEDECL(octaveAmps, "Noise Octave Amplitudes") = ShConstAttrib2f(0.5f, 0.25f);

    ShProgram bander = SH_BEGIN_PROGRAM() {
      ShInputPoint3f SH_DECL(posm);

      posm *= bandFreq;
      ShAttrib1f inband = posm(0); 
      ShAttrib1f noise = noiseScale * sturbulence<1>(posm * noiseFreq, octaveAmps, useTexture);
      inband = frac(inband + noise); // add noise

      ShColor3f bandColor = lerp(deprecated_smoothstep(bandrange(0), bandrange(1), inband), color1, color2);
      inband = deprecated_smoothstep(bandrange(0) - width1, bandrange(0), inband) - 
      deprecated_smoothstep(bandrange(1), bandrange(1) + width2, inband);

      ShOutputColor3f SH_DECL(kd) = lerp(inband, bandColor, outColor); 
      ShOutputColor3f SH_DECL(ks) = specular; 
      ShOutputAttrib1f SH_DECL(specExp) = exponent;
    } SH_END_PROGRAM;
    fsh = fsh << bander;
  }
};

SimpleWoodNoise simple_wood_noise(true);
ComplexWoodNoise complex_wood_noise(true);
MarbleNoise marble_noise(true);
