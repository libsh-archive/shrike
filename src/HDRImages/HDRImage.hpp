#ifndef HDR_IMAGE_H
#define HDR_IMAGE_H

#include <string>
#include <sh/sh.hpp>

using namespace std;
using namespace SH;

class HDRImage {
public:
	HDRImage();
  HDRImage(int width, int height, int depth);
	HDRImage(const HDRImage& other);
	~HDRImage();

	HDRImage& operator=(const HDRImage& other);

	/*
	void loadEXR(const char filename[]); // load an OpenEXR file
	void saveEXR(const char filename[]); // save an OpenEXR file
	*/
	
	void loadHDR(const char filename[]); // load a HDR file
	void saveHDR(const char filename[]); // save a HDR file

	void CreateHDRCubeMap(const char filename[]); // save a cubemap
	
  float operator()(int x, int y, int i) const { return m_image(x,y,i); }
  float& operator()(int x, int y, int i) { return m_image(x,y,i); }
	
	int width() const { return m_width; }
	int height() const { return m_height; }
	int elements() const { return m_elements; }
  
	void loadPng(const std::string& filename) { m_image.loadPng(filename); }
	
	void savePng(const std::string& filename) { m_image.savePng(filename); }  
  
	void savePng16(const std::string& filename) { m_image.savePng16(filename); }

	ShImage getNormalImage() { return m_image.getNormalImage(); }

  const float* data() const { return m_image.data(); }
  float* data() { return m_image.data(); }
	
	void dirty() { m_image.dirty(); }
	ShMemoryPtr	memory() { return m_image.memory(); }
	ShPointer<const ShMemory>	memory() const { return m_image.memory(); }
	
	ShImage image() { return m_image; }
	
private:
	void ComputeCubeMap();
	ShImage m_image;
	ShImage m_cubemapimage;
	int m_width, m_height;
  int m_elements;
};

#endif
