# OGLFT_WITH_OGLFT_DIR
# Adds a --with-oglft option to specify the OGLFT installation directory
AC_DEFUN([OGLFT_WITH_OGLFT_DIR], [
  AC_ARG_WITH([oglft], AC_HELP_STRING([--with-oglft=DIR],
			              [specify that OGLFT is installed in DIR]),
    [CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],
    [])
])
