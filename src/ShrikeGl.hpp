#ifndef SH_SHRIKEGL_HPP
#define SH_SHRIKEGL_HPP

#ifdef WIN32
#include <windows.h>
#endif

#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY

#ifdef WIN32

extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
extern PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB;
extern PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB;
extern PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
extern PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB;

#endif

void shrikeGlInit();

#endif
