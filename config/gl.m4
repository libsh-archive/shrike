# Various checks for GL

# GL_WITH_GL_DIR
# Adds a --with-gl option to specify the GL installation directory
AC_DEFUN([GL_WITH_GL_DIR], [
  AC_ARG_WITH([gl], AC_HELP_STRING([--with-gl=DIR], [specify that OpenGL is installed in DIR]),
    [CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],
    [])
])

# GL_CHECK_GL_HEADERS(if-available, if-not-available)
# Finds GL headers in GL/ or OpenGL/.
#
AC_DEFUN([GL_CHECK_GL_HEADERS], [

  # If we are OSX, OpenGL is included and we need -framework OpenGL
  case $host_os in
  darwin*)
    AC_MSG_NOTICE([using Apple OpenGL framework])
    AC_DEFINE([HAVE_GL_GL_H], [1])
    AC_DEFINE([HAVE_GL_GLEXT_H], [1])
    have_gl_gl_h=true
    have_gl_glext_h=true
    gl_gl_dir=OpenGL
    gl_glext_dir=OpenGL
    GL_LIBS="-framework OpenGL -framework AGL"
    ;;
  # other systems
  *)
    # First check for GL/gl.h and GL/glext.h
    AC_CHECK_HEADERS([GL/gl.h], [have_gl_gl_h=true gl_gl_dir=GL], [have_gl_gl_h=false])
    AC_CHECK_HEADERS([GL/glext.h], [have_gl_glext_h=true gl_glext_dir=GL], [have_gl_glext_h=false],
      [#if HAVE_GL_GL_H
      #include <GL/gl.h>
    #endif])
    GL_LIBS="-lGL"
    ;;
  esac

  if test "x$have_gl_gl_h" = "xtrue" -a "x$have_gl_glext_h" = "xtrue" ; then
    found_gl_headers=true
  fi

  # Finally check if we've found any headers at all and act appropriately.
  if test "x$found_gl_headers" = "xtrue" ; then
    :; $1
  else
    :; $2
  fi
])

# GL_CHECK_GLEXT_VERSION(version, if-avail, if-not-avail)
#
AC_DEFUN([GL_CHECK_GLEXT_VERSION], [
  AC_MSG_CHECKING([for glext.h version >= $1])
  AC_RUN_IFELSE(
    AC_LANG_PROGRAM([#if HAVE_GL_GLEXT_H
# include <$gl_gl_dir/gl.h>
# include <$gl_glext_dir/glext.h>
#else
# define GL_GLEXT_VERSION 0
#endif],
    [#if GL_GLEXT_VERSION >= $1
  return 0;
#else
  return 1;
#endif]),
  [AC_MSG_RESULT([yes])
   $2],
  [AC_MSG_RESULT([no])
   $3])
])

