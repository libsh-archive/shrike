bin_PROGRAMS = shrike

shrike_SOURCES = ShrikeApp.hpp ShrikeApp.cpp \
		 ShrikeFrame.hpp ShrikeFrame.cpp \
		 ShrikeCanvas.hpp ShrikeCanvas.cpp \
		 Shader.hpp Shader.cpp \
		 UniformPanel.hpp UniformPanel.cpp \
		 ShObjFile.hpp ShObjFile.cpp \
		 Globals.hpp Globals.cpp \
		 Camera.hpp Camera.cpp \
		 ShTrackball.hpp ShTrackball.cpp \
		 DiffuseShader.cpp \
		 PhongShader.cpp \
		 WorleyShader.cpp \
		 TexShader.cpp \
		 Ashikhmin.cpp 

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall
AM_CPPFLAGS = `${WX_CONFIG} --cppflags`
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shrike_LDADD = -lsh -lshutil
