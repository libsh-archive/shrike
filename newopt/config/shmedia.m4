
AC_DEFUN([SHMEDIA_WITH_SHMEDIA_DIR], [
  AC_ARG_WITH([shmedia], AC_HELP_STRING([--with-shmedia=DIR],
	      [specify that shmedia is in DIR (default=PREFIX/share/shmedia)]),
	      [shmedia_dir="${withval}"],
              [if test x${prefix} != xNONE ; then
		 shmedia_dir="${prefix}/share/shmedia"
	       else
		 shmedia_dir="`pwd`/${srcdir}/../shmedia"
	       fi])
  AC_DEFINE_UNQUOTED([SHMEDIA_DIR], "${shmedia_dir}", [shmedia install location])
])

