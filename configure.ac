AC_INIT([Puttle], [1.0.0], [moncelier@devlife.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS(config.h)
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_LIB([boost_regex], [main], [LIBS="$LIBS -lboost_regex"], [AC_MSG_ERROR([Not Found: boost_regex])])
AC_CHECK_LIB([boost_system], [main], [LIBS="$LIBS -lboost_system"], [AC_MSG_ERROR([Not Found: boost_system])])
AC_CHECK_LIB([boost_thread], [main], [LIBS="$LIBS -lboost_thread"], [AC_MSG_ERROR([Not Found: boost_thread])])
AC_CHECK_LIB([boost_program_options], [main], [LIBS="$LIBS -lboost_program_options"], [AC_MSG_ERROR([Not Found: boost_program_options])])

PKG_CHECK_MODULES(LOG4CPP, log4cpp)
PKG_CHECK_MODULES(OPENSSL, openssl >= 0.9.6)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_CHECK_HEADERS([tr1/cstdint])
AC_CHECK_HEADERS([boost/cstdint.hpp])
AC_LANG_RESTORE

dnl enable debug
AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE(debug,
			  AS_HELP_STRING([--enable-debug], [build with debugging symbols @<:@default=no@:>@]),
			  [case "${enableval}" in
			   yes | no ) enable_debug="${enableval}" ;;
		       *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
		      esac],
			  [enable_debug="no"]
)
AC_MSG_RESULT([$debugit])

AM_CONDITIONAL(HAVE_DEBUG, test "$enable_debug" = yes)
if test "$enable_debug" = yes; then
	AM_CFLAGS="-g -Wall -Werror -Wno-uninitialized -O0"
	AM_CXXFLAGS="-g -Wall -Werror -Wno-uninitialized -O0"
	AC_DEFINE(HAVE_DEBUG, 1, [Define to 1 if you want debug.])
fi

AM_CXXFLAGS="$AM_CXXFLAGS $OPENSSL_CFLAGS -std=gnu++0x"
LIBS="$LIBS $OPENSSL_LIBS $LOG4CPP_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST([LIBS])

SHAVE_INIT([shave/shave], [enable])
AC_CONFIG_FILES(
				Makefile
				src/Makefile
				tests/Makefile
				shave/shave/shave
				shave/shave/shave-libtool)


AC_OUTPUT
