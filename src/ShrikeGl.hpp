#ifndef SH_SHRIKEGL_HPP
#define SH_SHRIKEGL_HPP

#ifdef WIN32
#include <windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

#if defined( __APPLE__ )
#define GL_GLEXT_VERBOSE 1
#define GL_GLEXT_PROTOTYPES 1
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_LEGACY
#endif

#if defined( WIN32 ) | defined( __APPLE_ )

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
