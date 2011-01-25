AM_CXXFLAGS = -I$(top_srcdir)/src @AM_CXXFLAGS@

check_PROGRAMS = tests
TESTS = tests
TESTS_ENVIRONMENT= srcdir=$(srcdir)


tests_SOURCES = tests.cpp ../src/authenticator.cpp ../src/proxy.cpp \
				\
				test-authenticator.h

tests_LDADD = @LIBS@

