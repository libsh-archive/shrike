# shgenmap is a tool to generate 16 bit png files to use as bumpmaps, not sure
# if we want to keep it in this directory
bin_PROGRAMS = shrike shgenmap shHDRfunc shsparse

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
shrike_SOURCES += shaders/HomomorphicShaderInterp.cpp
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
shrike_SOURCES += shaders/SparseTexture.cpp
shrike_SOURCES += shaders/FragmentLooping.cpp
shrike_SOURCES += HDRImages/HDRImage.cpp
shrike_SOURCES += shaders/SpatialBRDF.cpp
shrike_SOURCES += shaders/HDRViewer.cpp
shrike_SOURCES += shaders/HDRMipMap.cpp
shrike_SOURCES += shaders/HDRInterp.cpp
shrike_SOURCES += shaders/HDRToneMapping.cpp
shrike_SOURCES += shaders/HDRWrap.cpp
shrike_SOURCES += shaders/CubeMap.cpp
shrike_SOURCES += shaders/HDRLucite.cpp shaders/HDRGlass.cpp shaders/HDRShinyBumpMap.cpp
shrike_SOURCES += shaders/EdgeDetection.cpp
shrike_SOURCES += shaders/CannyEdgeDetector.cpp
shrike_SOURCES += shaders/GaussianBlur.cpp
shrike_SOURCES += shaders/AnisotropicDiffusion.cpp
shrike_SOURCES += shaders/LargeTexture.cpp
shrike_SOURCES += shaders/ClipMap.cpp

# HDR headers
shrike_SOURCES += HDRImages/HDRImage.hpp HDRImages/Filters.hpp HDRImages/HDRInterp.hpp
shrike_SOURCES += HDRImages/HDRMipmap.hpp HDRImages/HDRProbe.hpp HDRImages/HDRTonemap.hpp
shrike_SOURCES += HDRImages/LargeTexture.hpp HDRImages/HDRCubeMap.hpp
shrike_SOURCES += shaders/ClipMap.hpp

AM_CXXFLAGS = `${WX_CONFIG} --cxxflags` -Wall
AM_CPPFLAGS = `${WX_CONFIG} --cppflags`
shrike_CPPFLAGS = -IHDRImages
shrike_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shrike_LDADD = -lsh -lshutil

shgenmap_SOURCES = ShGenMap.cpp
shgenmap_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shgenmap_LDADD = -lsh -lshutil

shHDRfunc_SOURCES = ShHDRFunc.cpp HDRImages/HDRImage.cpp HDRImages/HDRImage.hpp
shHDRfunc_CPPFLAGS = -IHDRImages
shHDRfunc_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shHDRfunc_LDADD = -lsh -lshutil

shsparse_SOURCES = ShSparse.cpp HDRImages/HDRImage.cpp HDRImages/HDRImage.hpp
shsparse_CPPFLAGS = -IHDRImages
shsparse_LDFLAGS = `${WX_CONFIG} --libs --gl-libs`
shsparse_LDADD = -lsh -lshutil
