AM_CXXFLAGS = -I$(top_srcdir)/src @AM_CXXFLAGS@

bin_PROGRAMS = puttle

puttle_SOURCES = puttle_server.cpp puttle_proxy.cpp authenticator.cpp main.cpp logger.cpp proxy.cpp \
				 puttle_server.h puttle_proxy.h authenticator.h logger.h proxy.h singleton.h \
				 puttle-common.h

puttle_LDADD = @LIBS@

if HAVE_DEBUG
check-local:
	$(MAKE) -C $(top_srcdir)/tests check
endif
