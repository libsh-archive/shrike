#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"
#include "HDRCubeMap.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class HDRGlassShader : public Shader {
public:
  ShProgram vsh, fsh;

  HDRGlassShader();
  ~HDRGlassShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	std::string fname;
};

HDRGlassShader::HDRGlassShader()
  : Shader("HDR: Glass Refraction"), fname("Bf_cubemap.hdr")
{
	setStringParam("Image Name", fname);
}

HDRGlassShader::~HDRGlassShader()
{
}

bool HDRGlassShader::init()
{
	HDRImage image;
	std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
	image.loadHDR(filename.c_str());
	CubeMap<ShUnclamped<ShTextureRect<ShVector4f> > > Img(image.width(), image.height());
	Img.internal(true);
	Img.memory(image.memory());

  ShAttrib1f theta = ShAttrib1f(1.3f);
  theta.name("relative indices of refraction");
  theta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShOutputNormal3f onorm;  // view-space normal
    ShOutputVector3f reflv; // Compute reflection vector
    ShOutputVector3f refrv; // Compute refraction vector
    ShOutputAttrib1f fres; // Compute fresnel term

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShVector3f viewv = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector
    refrv = refract(viewv,onorm,theta); // Compute refraction vector
    fres = fresnel(viewv,onorm,theta); // Compute fresnel term

    // actually do reflection and refraction lookup in model space
    reflv = Globals::mv_inverse | reflv;
    refrv = Globals::mv_inverse | refrv;
  } SH_END;

	ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
	level.range(-10.0,10.0);  
  
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f n;  // normal
    ShInputVector3f reflv; // Compute reflection vector
    ShInputVector3f refrv; // Compute refraction vector
    ShInputAttrib1f fres; // Compute fresnel term

    ShOutputColor3f result;
    
    result = fres*Img(reflv)(0,1,2) + (1.0f-fres)*Img(refrv)(0,1,2); 

		// display the image
		level = pow (2, level + 2.47393);
		ShAttrib3f RGB = level * result(0,1,2);
		
		ShAttrib1f f = 0.184874;
		ShAttrib1f e = 2.718281828;
		ShAttrib1f R = cond(RGB(0)>1.0, 1.0 + log2((RGB(0)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(0));
		ShAttrib1f G = cond(RGB(1)>1.0, 1.0 + log2((RGB(1)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(1));
		ShAttrib1f B = cond(RGB(2)>1.0, 1.0 + log2((RGB(2)-1.0) * f + 1.0) * rcp(f*log2(e)), RGB(2));
		ShAttrib1f gammainv = 0.454545455; // gamma-correction = 1/2.2
		R = pow(R,gammainv);
		G = pow(G,gammainv);
		B = pow(B,gammainv);
		
		result	= ShColor3f(R,G,B) * 0.285714286; // scale to get the correct range	(0.285714286=1/3.5)
  } SH_END;

  return true;
}


HDRGlassShader the_hdr_glass_shader;
