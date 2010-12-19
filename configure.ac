AC_INIT([puttle], 1.0)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_LIB([boost_system], [main], [LIBS="$LIBS -lboost_system"], [AC_MSG_ERROR([Not Found: boost_system])])
AC_CHECK_LIB([boost_thread], [main], [LIBS="$LIBS -lboost_thread"], [AC_MSG_ERROR([Not Found: boost_thread])])

AC_SUBST([LIBS])

SHAVE_INIT([shave/shave], [enable])
AC_CONFIG_FILES(
				Makefile
				src/Makefile
				shave/shave/shave
				shave/shave/shave-libtool)


AC_OUTPUT
