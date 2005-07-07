// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA  02110-1301, USA
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
