#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <cmath>
#include "Shader.hpp"
#include "Globals.hpp"
#include "HDRImage.hpp"
#include "HDRCubeMap.hpp"
#include "HDRInterp.hpp"
#include "HDRMipmap.hpp"

using namespace SH;
using namespace ShUtil;

#include "util.hpp"

class testCubeMap : public Shader {
public:
  testCubeMap();
  ~testCubeMap();

  void createMaps(const ShImage&, ShImage&, ShImage&);
  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

	std::string fname;

  static testCubeMap instance;
};

testCubeMap::testCubeMap()
  : Shader("HDR: Cube map"), fname("Af_cubemap.hdr")
{
	setStringParam("Image Name", fname);
}

testCubeMap::~testCubeMap()
{
}

bool testCubeMap::init()
{
	HDRImage image;
	std::string filename = SHMEDIA_DIR "/hdr/hdr/" + fname;
	image.loadHDR(filename.c_str());
	CubeMap<MipMap<ShUnclamped<ShTextureRect<ShVector4f> > > > Img(image.width(), image.height());
	Img.internal(true);
	Img.memory(image.memory());
	Img.updateMipMap();


  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;
    ShOutputVector3f view;
		ShOutputVector3f reflv;
		
    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

		ShPoint3f posv = (Globals::mv | ipos)(0,1,2); // Compute view-space position
    ShPoint3f viewv = -normalize(posv); // Compute view vector
    view = normalize(viewv - posv);

		reflv = reflect(viewv,onorm); // Compute reflection vector
    // actually do reflection lookup in model space
    reflv = Globals::mv_inverse | reflv;	
} SH_END;

	ShAttrib1f SH_DECL(level) = ShAttrib1f(0.0);
	level.range(-10.0,10.0);

	ShAttrib2f SH_DECL(scale) = ShAttrib2f(1.0,1.0);
	scale.range(0.1,10.0);

	ShAttrib1f SH_DECL(mipmaplevel) = ShAttrib1f(0.0);
	mipmaplevel.range(0.0,8.0);
	mipmaplevel.name("mipmap level");
	
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f tc;
    ShInputNormal3f normal;
		ShInputVector3f view;
		ShInputVector3f refl;
 
		ShOutputColor3f result;

		Img.setLevel(mipmaplevel);
		ShVector4f interp = Img(refl);

		// display the image
		level = pow (2, level + 2.47393);
		ShAttrib3f RGB = level * interp(0,1,2);
		
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

testCubeMap testCubeMap::instance = testCubeMap();
