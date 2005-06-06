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
    GET_WGL_PROCEDURE(glMultiTexCoord1fARB,
		      GLMULTITEXCOORD1FARB);
  }
  if (!glMultiTexCoord2fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord2fARB,
		      GLMULTITEXCOORD2FARB);
  }
  if (!glMultiTexCoord3fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord3fARB,
		      GLMULTITEXCOORD3FARB);
  }
  if (!glMultiTexCoord4fARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord4fARB,
		      GLMULTITEXCOORD4FARB);
  }
  if (!glMultiTexCoord1fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord1fvARB,
		      GLMULTITEXCOORD1FVARB);
  }
  if (!glMultiTexCoord2fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord2fvARB,
		      GLMULTITEXCOORD2FVARB);
  }
  if (!glMultiTexCoord3fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord3fvARB,
		      GLMULTITEXCOORD3FVARB);
  }
  if (!glMultiTexCoord4fvARB) {
    GET_WGL_PROCEDURE(glMultiTexCoord4fvARB,
		      GLMULTITEXCOORD4FVARB);
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
