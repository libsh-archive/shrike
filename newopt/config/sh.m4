# SH_CHECK_BACKEND(BACKEND_NAME, DESCRIPTIVE_NAME, DEFAULT=true)
AC_DEFUN([SH_CHECK_BACKEND],
[AC_ARG_ENABLE([$1-backend],
               AC_HELP_STRING([--enable-$1-backend],
                              [compile $2 backend (default=yes)]),
               [case "${enableval}" in
	         yes) ac_backend_$1=true ;;
		 no)  ac_backend_$1=false ;;
                 *) AC_MSG_ERROR(bad value ${enableval} for --enable-$1-backend) ;;
              esac],[ac_backend_$1=m4_default([$3], true)])
])

# SH_WITH_SH_DIR
# Adds a --with-sh option to specify the Sh installation directory
AC_DEFUN([SH_WITH_SH_DIR], [
  AC_MSG_CHECKING([where Sh is installed])
  AC_ARG_WITH([sh], AC_HELP_STRING([--with-sh=DIR], [specify that Sh is installed in DIR]),
    [AC_MSG_RESULT([in ${withval}])
     CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],
    [AC_MSG_RESULT([in default directory])])
])

# SH_CHECK_SH_HEADERS(TRUE, FALSE)
# Checks whether Sh header files are present.
AC_DEFUN([SH_CHECK_SH_HEADERS], [
  AC_CHECK_HEADER([sh/sh.hpp], [$1],
    [$2])
])
