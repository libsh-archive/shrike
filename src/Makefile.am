# shgenmap is a tool to generate 16 bit png files to use as bumpmaps, not sure
# if we want to keep it in this directory
bin_PROGRAMS = shrike shgenmap

AUTOMAKE_OPTIONS = subdir-objects

shrike_SOURCES = ShrikeApp.hpp ShrikeApp.cpp \
		 ShrikeFrame.hpp ShrikeFrame.cpp \
		 ShrikeCanvas.hpp ShrikeCanvas.cpp \
		 Shader.hpp Shader.cpp \
		 UniformPanel.hpp UniformPanel.cpp \
		 ShrikePropsDialog.hpp ShrikePropsDialog.cpp \
		 Globals.hpp Globals.cpp \
		 Camera.hpp Camera.cpp \
		 ShTrackball.hpp ShTrackball.cpp \
		 Timer.hpp Timer.cpp \
		 ShrikeGl.cpp ShrikeGl.hpp \
		 Project.cpp Project.hpp \
		 AboutDialog.cpp AboutDialog.hpp \
		 Build.cpp Build.hpp

if SHRIKE_DYNAMIC_SHADERS

SUBDIRS = . shaders
shrike_SOURCES += shaders/LCDSmall.hpp shaders/LCDSmall.cpp

lib_LIBRARIES = libshrike.a
libshrike_a_SOURCES = Shader.hpp Shader.cpp

else
shrike_SOURCES += shaders/util.hpp
shrike_SOURCES += shaders/util.cpp 
shrike_SOURCES += shaders/DiffuseShader.cpp
shrike_SOURCES += shaders/DiscoShader.cpp
shrike_SOURCES += shaders/PhongShader.cpp
shrike_SOURCES += shaders/SimplePhong.cpp
shrike_SOURCES += shaders/SimpleDiffuse.cpp
shrike_SOURCES += shaders/WorleyShader.cpp
shrike_SOURCES += shaders/TexShader.cpp
shrike_SOURCES += shaders/Ashikhmin.cpp
shrike_SOURCES += shaders/Texter.cpp
shrike_SOURCES += shaders/Text.cpp shaders/Text.hpp
shrike_SOURCES += shaders/WobbleShader.cpp
shrike_SOURCES += shaders/Logo.cpp
shrike_SOURCES += shaders/NonPeriodic.cpp
shrike_SOURCES += shaders/NonPeriodic3D.cpp
shrike_SOURCES += shaders/NoiseShader.cpp
shrike_SOURCES += shaders/Textures.cpp
shrike_SOURCES += shaders/Pants.cpp
shrike_SOURCES += shaders/EnvMapShader.cpp
shrike_SOURCES += shaders/ShinyBumpMapShader.cpp
shrike_SOURCES += shaders/HomomorphicShader.cpp
shrike_SOURCES += shaders/SatinShader.cpp
shrike_SOURCES += shaders/JeweledShader.cpp
shrike_SOURCES += shaders/AlgebraShader.cpp
shrike_SOURCES += shaders/GlassShader.cpp
shrike_SOURCES += shaders/LuciteShader.cpp
shrike_SOURCES += shaders/CookTorrance.cpp
shrike_SOURCES += shaders/Banks.cpp
shrike_SOURCES += shaders/BrickWall.cpp
shrike_SOURCES += shaders/BrickHorizon.cpp
shrike_SOURCES += shaders/HorizonMapping.cpp
shrike_SOURCES += shaders/HairShader.cpp
shrike_SOURCES += shaders/LCD.hpp shaders/LCD.cpp
shrike_SOURCES += shaders/LCDSmall.hpp shaders/LCDSmall.cpp
shrike_SOURCES += shaders/LafortuneShader.cpp
shrike_SOURCES += shaders/Derivatives.cpp
shrike_SOURCES += shaders/Checkerboard.cpp
shrike_SOURCES += shaders/VertexBranching.cpp
shrike_SOURCES += shaders/Tangents.cpp
shrike_SOURCES += shaders/TangentArrows.cpp
shrike_SOURCES += shaders/BumpMapShader.cpp
shrike_SOURCES += shaders/FragmentBranching.cpp
shrike_SOURCES += shaders/FragmentLooping.cpp
shrike_SOURCES += shaders/PaletteExample.cpp
endif

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall
AM_CPPFLAGS = `${WX_CONFIG} --cppflags`
shrike_CPPFLAGS = -DSHRIKE_LIB_DIR=\"$(prefix)/lib/shrike\"
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shrike_LDADD = $(GL_LIBS) -lsh -lshutil

shgenmap_SOURCES = ShGenMap.cpp
shgenmap_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shgenmap_LDADD = -lsh -lshutil
