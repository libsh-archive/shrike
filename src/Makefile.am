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
		 util.hpp util.cpp utilimpl.hpp

shrike_SOURCES += shaders/DiffuseShader.cpp
shrike_SOURCES += shaders/DiscoShader.cpp
shrike_SOURCES += shaders/PhongShader.cpp
shrike_SOURCES += shaders/SimplePhong.cpp
shrike_SOURCES += shaders/SimpleDiffuse.cpp
shrike_SOURCES += shaders/WorleyShader.cpp
shrike_SOURCES += shaders/TexShader.cpp
#shrike_SOURCES += shaders/CelShader.cpp
shrike_SOURCES += shaders/Ashikhmin.cpp
shrike_SOURCES += shaders/Texter.cpp
shrike_SOURCES += shaders/Text.cpp shaders/Text.hpp
shrike_SOURCES += shaders/WobbleShader.cpp
shrike_SOURCES += shaders/FrameMapping.cpp
shrike_SOURCES += shaders/Logo.cpp
shrike_SOURCES += shaders/NonPeriodic.cpp
shrike_SOURCES += shaders/NonPeriodic3D.cpp
shrike_SOURCES += shaders/NoiseShader.cpp
shrike_SOURCES += shaders/EdgeGooch.cpp
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
shrike_SOURCES += shaders/LafortuneShader.cpp
shrike_SOURCES += shaders/LCD.cpp
shrike_SOURCES += shaders/HairPhysics.cpp

# this is a hack and should be removed eventually
# shrike_SOURCES += DummyShader.cpp

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall
AM_CPPFLAGS = `${WX_CONFIG} --cppflags`
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shrike_LDADD = -lsh -lshutil

shgenmap_SOURCES = ShGenMap.cpp
shgenmap_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shgenmap_LDADD = -lsh -lshutil
