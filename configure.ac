AC_INIT([Carona USP], [0.1], [tiago.shibata@gmail.com])
# Para depuração:
# -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4 -­D_FORTIFY_SOURCE=2 -O2 -g
# ou -O0 -g
: ${CXXFLAGS="-O3 -flto"}
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_PROG_CXX(g++)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
])

AX_CHECK_COMPILE_FLAG([-fdiagnostics-color=auto],
    [CXXFLAGS="$CXXFLAGS -fdiagnostics-color=auto"])

AC_OUTPUT
