// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
#ifndef SH_SHRIKEGL_HPP
#define SH_SHRIKEGL_HPP

#ifdef WIN32
# include <windows.h>
#endif

#ifdef __APPLE__
# define GL_GLEXT_VERBOSE 1
# define GL_GLEXT_PROTOTYPES 1
# include <OpenGL/gl.h>
# include <OpenGL/glext.h>
#else
# define GL_GLEXT_LEGACY
# include <GL/gl.h>
# ifndef WIN32
#  define GL_GLEXT_PROTOTYPES
# endif
# include <GL/glext.h>
# undef GL_GLEXT_LEGACY
#endif

#if defined(WIN32) 

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
