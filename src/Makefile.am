bin_PROGRAMS = shrike

shrike_SOURCES = ShrikeApp.hpp ShrikeApp.cpp \
		 ShrikeFrame.hpp ShrikeFrame.cpp \
		 ShrikeCanvas.hpp ShrikeCanvas.cpp \
		 Shader.hpp Shader.cpp \
		 UniformPanel.hpp UniformPanel.cpp \
		 ShrikePropsDialog.hpp ShrikePropsDialog.cpp \
		 Globals.hpp Globals.cpp \
		 Camera.hpp Camera.cpp \
		 ShTrackball.hpp ShTrackball.cpp \
		 DiffuseShader.cpp \
		 DiscoShader.cpp \
		 PhongShader.cpp \
		 SimplePhong.cpp \
		 SimpleDiffuse.cpp \
		 WorleyShader.cpp \
		 TexShader.cpp \
	         CelShader.cpp \
		 Ashikhmin.cpp \
		 Texter.cpp \
		 Text.cpp Text.hpp \
		 WobbleShader.cpp \
		 FrameMapping.cpp \
		 Logo.cpp \
		 NonPeriodic.cpp \
		 Textures.cpp \
		 Pants.cpp

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall
AM_CPPFLAGS = `${WX_CONFIG} --cppflags`
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shrike_LDADD = -lsh -lshutil
