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
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord1fARB = 0;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord2fARB = 0;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord3fARB = 0;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord4fARB = 0;
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord1fvARB = 0;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord2fvARB = 0;
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord3fvARB = 0;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord4fvARB = 0;
#endif
