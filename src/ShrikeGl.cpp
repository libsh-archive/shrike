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
#include "ShrikeGl.hpp"

#ifdef WIN32
#define GET_WGL_PROCEDURE(x, T) do { x = reinterpret_cast<PFN ## T ## PROC>(wglGetProcAddress(#x)); } while(0)
#else
#define GET_WGL_PROCEDURE(x, T) do { } while (0)
#endif

void shrikeGlInit()
{
#ifdef WIN32
  if (!glMultiTexCoord1fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord1fARB, GLMULTITEXCOORD1FARB);
  }
  if (!glMultiTexCoord2fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord2fARB, GLMULTITEXCOORD2FARB);
  }
  if (!glMultiTexCoord3fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord3fARB, GLMULTITEXCOORD3FARB);
  }
  if (!glMultiTexCoord4fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord4fARB, GLMULTITEXCOORD4FARB);
  }
  if (!glMultiTexCoord1fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord1fvARB, GLMULTITEXCOORD1FVARB);
  }
  if (!glMultiTexCoord2fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord2fvARB, GLMULTITEXCOORD2FVARB);
  }
  if (!glMultiTexCoord3fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord3fvARB, GLMULTITEXCOORD3FVARB);
  }
  if (!glMultiTexCoord4fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord4fvARB, GLMULTITEXCOORD4FVARB);
  }
#endif
}

#ifdef WIN32
PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB = 0;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = 0;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB = 0;
PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB = 0;
PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB = 0;
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB = 0;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB = 0;
PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB = 0;
#endif
