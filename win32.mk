# install location
SH_INSTALLDIR = \dev\install
SHMEDIA_DIR = \"\\\\dev\\\\shmedia\"

# wxWindows
WXWIN_DIR = \dev\wxWindows-2.4.2
INCLUDES += /I$(WXWIN_DIR)\include /I$(WXWIN_DIR)\lib\mswd
WXWIN_RELEASE_LDADD = $(WXWIN_DIR)\lib\wxmsw.lib
WXWIN_DEBUG_LDADD = $(WXWIN_DIR)\lib\wxmswd.lib

CXX = cl /nologo
AR = link /lib /nologo
LD = link /nologo

CPPFLAGS = /DWIN32 /DNOMINMAX /D_USE_MATH_DEFINES
CPPFLAGS += /DSHMEDIA_DIR=$(SHMEDIA_DIR)
CPPFLAGS += $(INCLUDES)
CXXFLAGS = /GR /GX /wd4003

RELEASE_CPPFLAGS = $(CPPFLAGS)
RELEASE_CXXFLAGS = $(CXXFLAGS) $(RELEASE_CPPFLAGS) /MD /O2

DEBUG_CPPFLAGS = $(CPPFLAGS) /D_DEBUG /DSH_DEBUG
DEBUG_CXXFLAGS = $(CXXFLAGS) $(DEBUG_CPPFLAGS) /MDd /Zi /Od

LDFLAGS = 
RELEASE_LDFLAGS = $(LDFLAGS)
DEBUG_LDFLAGS = $(LDFLAGS) /DEBUG

clean:
	del *.lib *.obj *.d $(CLEANFILES)

%.r.obj: %.cpp
	$(CXX) /Fo$@ /c $< $(RELEASE_CXXFLAGS)

%.d.obj: %.cpp
	$(CXX) /Fo$@ /c $< $(DEBUG_CXXFLAGS)

%.r.d: %.cpp
	@makedepend -f- -o.r.obj $< -- $(RELEASE_CPPFLAGS) > $@ 2>devnull
	-@del devnull

%.d.d: %.cpp
	@makedepend -f- -o.d.obj $< -- $(RELEASE_CPPFLAGS) > $@ 2>devnull
	-@del devnull
