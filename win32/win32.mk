SH_INSTALLDIR = \Dev\install-sh
OPENGL_DIR = \dev\opengl
MSSDK_DIR = \dev\mssdk
LIBPNG_DIR = \dev\libpng
ZLIB_DIR = \dev\zlib

# libpng/zlib
INCLUDES = -I$(LIBPNG_DIR)\include -I$(ZLIB_DIR)\include
# MS platform SDK
INCLUDES += -I$(MSSDK_DIR)\include
# OpenGL
INCLUDES += -I$(OPENGL_DIR)\include

CPPFLAGS = -DWIN32 $(INCLUDES)

RELEASE_CPPFLAGS = $(CPPFLAGS)
DEBUG_CPPFLAGS   = $(CPPFLAGS) /D SH_DEBUG /D _DEBUG

CXXFLAGS = /GR /GX /wd4003
RELEASE_CXXFLAGS = $(CXXFLAGS) $(RELEASE_CPPFLAGS)  /MD
DEBUG_CXXFLAGS = $(CXXFLAGS) $(DEBUG_CPPFLAGS) /MDd /ZI /Od /RTC1

LDFLAGS = 
DEBUG_LDFLAGS = $(LDFLAGS) /Zi
RELEASE_LDFLAGS = $(LDFLAGS)

CXX = cl

clean:
	del *.lib *.obj *.d $(CLEANFILES)

%.r.obj: %.cpp
	$(CXX) /Fo$@ /c $< $(RELEASE_CXXFLAGS)

%.d.obj: %.cpp
	$(CXX) /Fo$@ /c $< $(DEBUG_CXXFLAGS)

%.r.d: %.cpp
	makedepend -f- -o.r.obj $< -- $(RELEASE_CPPFLAGS) > $@ 2>devnull
	-@del devnull

%.d.d: %.cpp
	makedepend -f- -o.d.obj $< -- $(RELEASE_CPPFLAGS) > $@ 2>devnull
	-@del devnull
