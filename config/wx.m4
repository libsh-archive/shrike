AC_DEFUN([WX_FIND_WX_CONFIG], [
  AC_PATH_PROG(WX_CONFIG, wx-config, no)
  if [[ x"${WX_CONFIG}" = "xno" ]] ; then
    AC_MSG_ERROR([shrike requires WxWindows. Could not find wx-config.])
  fi
])
