# shgenmap is a tool to generate 16 bit png files to use as bumpmaps, not sure
# if we want to keep it in this directory
bin_PROGRAMS = shrike # shgenmap

shrike_SOURCES = ShrikeApp.hpp ShrikeApp.cpp \
		 ShrikeFrame.hpp ShrikeFrame.cpp \
		 ShrikeCanvas.hpp ShrikeCanvas.cpp \
		 Shader.hpp Shader.cpp \
		 UniformPanel.hpp UniformPanel.cpp \
		 ShrikePropsDialog.hpp ShrikePropsDialog.cpp \
		 Globals.hpp Globals.cpp \
		 Camera.hpp Camera.cpp \
		 ShTrackball.hpp ShTrackball.cpp \
		 GrView.hpp GrView.cpp \
		 GrNode.hpp GrNode.cpp \
		 GrPort.hpp GrPort.cpp \
		 GrGen.hpp GrGen.cpp \
		 GrReverse.hpp GrReverse.cpp \
		 GrMonitor.hpp GrMonitor.cpp \
		 GrProgramMenu.hpp GrProgramMenu.cpp \
		 GrLayout.cpp \
		 util.cpp

#shrike_SOURCES += DiffuseShader.cpp
#shrike_SOURCES += DiscoShader.cpp
#shrike_SOURCES += PhongShader.cpp
shrike_SOURCES += SimplePhong.cpp
shrike_SOURCES += SimpleDiffuse.cpp
#shrike_SOURCES += WorleyShader.cpp
#shrike_SOURCES += TexShader.cpp
#shrike_SOURCES += CelShader.cpp
#shrike_SOURCES += Ashikhmin.cpp
#shrike_SOURCES += Texter.cpp
#shrike_SOURCES += Text.cpp Text.hpp
shrike_SOURCES += WobbleShader.cpp
#shrike_SOURCES += FrameMapping.cpp
#shrike_SOURCES += Logo.cpp
#shrike_SOURCES += NonPeriodic.cpp
#shrike_SOURCES += NoiseShader.cpp
#shrike_SOURCES += EdgeGooch.cpp
shrike_SOURCES += Textures.cpp
#shrike_SOURCES += Pants.cpp
#shrike_SOURCES += EnvMapShader.cpp
#shrike_SOURCES += ShinyBumpMapShader.cpp
#shrike_SOURCES += HomomorphicShader.cpp
#shrike_SOURCES += SatinShader.cpp
#shrike_SOURCES += AlgebraShader.cpp

# this is a hack and should be removed eventually
#shrike_SOURCES += DummyShader.cpp

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall `freetype-config --cflags`
AM_CPPFLAGS = `${WX_CONFIG} --cppflags` `freetype-config --cflags`
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs` `freetype-config --libs`
shrike_LDADD = -lsh -lshutil -lOGLFT -lfontconfig

# shgenmap_SOURCES = ShGenMap.cpp
# shgenmap_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
# shgenmap_LDADD = -lsh -lshutil
