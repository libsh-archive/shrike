AC_INIT(shrike, 0.7.10, libsh-devel@lists.libsh.org)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/ShrikeApp.cpp])
AM_CONFIG_HEADER(config.h)

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_LANG(C++)

SH_WITH_SH_DIR
SH_CHECK_SH_HEADERS


SHMEDIA_WITH_SHMEDIA_DIR

WX_CONFIG_WITH_WX_CONFIG
WX_FIND_WX_CONFIG
WX_CONFIG_GL_LIBS

GL_WITH_GL_DIR
GL_CHECK_GL_HEADERS

dnl Check if shaders should be loaded at runtime from DLLs
AC_ARG_ENABLE(dynamic-shaders,
[  --enable-dynamic-shaders Turn on dynamic shader loading],
[case "${enableval}" in
  yes) dynamic_shaders=true ;;
  no)  dynamic_shaders=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-dynamic-shaders) ;;
esac],[dynamic_shaders=false])
AM_CONDITIONAL(SHRIKE_DYNAMIC_SHADERS, test x$dynamic_shaders = xtrue)

AC_CONFIG_FILES([Makefile
		 src/Makefile
     src/shaders/Makefile])
AC_OUTPUT
