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

class HDRLuciteShader : public Shader {
public:
  ShProgram vsh, fsh;

  HDRLuciteShader();
  ~HDRLuciteShader();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

	std::string fname;
	
};

HDRLuciteShader::HDRLuciteShader()
	: Shader("HDR: Lucite Refraction"), fname("Af_cubemap.hdr")
{
	setStringParam("Image Name", fname);
}

HDRLuciteShader::~HDRLuciteShader()
{
}

bool HDRLuciteShader::init()
{
	HDRImage image;
	std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
	image.loadHDR(filename.c_str());
	CubeMap<ShUnclamped<ShTextureRect<ShVector4f> > > Img(image.width(), image.height());
	Img.internal(true);
	Img.memory(image.memory());

  ShAttrib3f theta = ShAttrib3f(1.32f,1.3f,1.28f);
  theta.name("relative indices of refraction");
  theta.range(0.0f,2.0f);

  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos;    // Position in NDC
    ShOutputNormal3f onorm;     // view-space normal
    ShOutputVector3f reflv;     // reflection vector
    ShOutputVector3f refrv[3];  // refraction vectors (per RGB channel)
    ShOutputAttrib3f fres;      // fresnel terms (per RGB channel)

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal
    onorm = normalize(onorm);
    ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector

    reflv = reflect(viewv,onorm); // Compute reflection vector

    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;

    for (int i=0; i<3; i++) {
    	refrv[i] = refract(viewv,onorm,theta[i]); // Compute refraction vectors

        // actually do refraction lookup in model space
        refrv[i] = Globals::mv_inverse | refrv[i];

        fres[i] = fresnel(viewv,onorm,theta[i]); // Compute fresnel term
    }
  } SH_END;

	ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
	level.range(-10.0,10.0);  

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputNormal3f n;  // normal
    ShInputVector3f reflv;     // reflection vector
    ShInputVector3f refrv[3];     // refraction vectors (per RGB channel)
    ShInputAttrib3f fres;         // fresnel terms (per RGB channel)

    ShOutputColor3f result;
    
    result = fres*Img(reflv)(0,1,2);
    for (int i=0; i<3; i++) {
        result[i] += (1.0f-fres[i])*Img(refrv[i])(i); 
    }
		
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


HDRLuciteShader the_hdr_lucite_shader;
