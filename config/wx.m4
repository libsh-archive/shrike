AC_DEFUN([WX_CONFIG_WITH_WX_CONFIG], [
  AC_ARG_WITH([wxconfig], AC_HELP_STRING([--with-wxconfig=FILE],
	      [specify the name of the wx-config program (default=wx-config)]),
	      [wx_config_binary="${withval}"],
	      [wx_config_binary="wx-config"])
  AC_DEFINE_UNQUOTED([WX_CONFIG_BINARY], "${wx_config_binary}", [wx-config program name])
])

AC_DEFUN([WX_FIND_WX_CONFIG], [
  AC_PATH_PROG(WX_CONFIG, "$wx_config_binary", no)
  if [[ x"${WX_CONFIG}" = "xno" ]] ; then
    AC_MSG_ERROR([shrike requires WxWindows. Could not find wx-config. You might need to use the --with-wxconfig option (e.g. ./configure --with-wxconfig=wx-config-2.4) if this program has a different name on your system.])
  fi
])

AC_DEFUN([WX_CONFIG_GL_LIBS], [
  AC_MSG_CHECKING([which GL libraries were compiled with WxWindows])
  gl_libs=`${WX_CONFIG} --gl-libs`
  if test x"${gl_libs}" = "x" ; then	
    AC_MSG_RESULT([none])
    AC_MSG_ERROR([WxWindows was not built with GL librairies. You must rebuild it.])
  else
    AC_MSG_RESULT(${gl_libs})
  fi
])