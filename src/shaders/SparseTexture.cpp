#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include "Shader.hpp"
#include "Globals.hpp"
#include <HDRImage.hpp>

using namespace SH;
using namespace ShUtil;


class SparseTexture : public Shader {
public:
  SparseTexture();
  ~SparseTexture();

  void createMaps(const ShImage&, ShImage&, ShImage&);
  bool init();
  
  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static SparseTexture instance;
};

SparseTexture::SparseTexture()
  : Shader("Sparse Texture")
{
}

SparseTexture::~SparseTexture()
{
}

bool SparseTexture::init()
{
	HDRImage data, map;
  data.loadHDR(SHMEDIA_DIR "/sparse/sparse_data.hdr");
  map.loadHDR(SHMEDIA_DIR "/sparse/sparse_map.hdr");
	
	int blocksize = (int)rint(map(0,0,2));
	HDRImage image(map.width()*blocksize,map.height()*blocksize,map.elements());
	for(int i=0 ; i<map.width() ; i++) {
		for(int j=0 ; j<map.height() ; j++) {
			for(int x=0 ; x<blocksize ; x++) {
				for(int y=0 ; y<blocksize ; y++) {
					for(int k=0 ; k<map.elements() ; k++) {
						image(i*blocksize+x,j*blocksize+y,k) = data((int)rint(map(i,j,0))+x,(int)rint(map(i,j,1))+y,k);
					}
				}	
			}
		}
	}
	
	
	ShTextureRect<ShColor4f> sparseTex(image.width(), image.height());
  sparseTex.memory(image.memory());
	
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    ShInputNormal3f inorm;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords
    ShOutputNormal3f onorm;

    opos = Globals::mvp | ipos; // Compute NDC position
    onorm = Globals::mv | inorm; // Compute view-space normal

  } SH_END;

	ShAttrib1f SH_DECL(softness) = ShAttrib1f(10.0);
	softness.name("shadow softness");
	softness.range(1.0,30.0);

  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputPosition4f posh;
    ShInputTexCoord2f u;
    ShInputNormal3f normal;
     
    ShOutputColor3f result;
	
		result = sparseTex(u)(0,1,2);
	} SH_END;
  return true;
}

SparseTexture SparseTexture::instance = SparseTexture();



